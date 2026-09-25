#include "SkillSlot.h"
#include "SkillHUD.h"
#include "imgui_internal.h"

// #define TextureDebug
#ifdef TextureDebug
#define TextureLog(...) RE::ConsoleLog::GetSingleton()->Print(__VA_ARGS__)
#else
#define TextureLog(...)
#endif

namespace AttunementSkillbar {

    void SkillSlot::SetConfiguration(SkillSlotConfiguration config) {
        _configuration = config;

        UpdateSkillTexture();
    }

    SkillSlotLocalFormConfiguration SkillSlot::LocalConfigurationWithFormID(RE::FormID formID) {
        auto form = RE::TESForm::LookupByID(formID);
        if (!form) {
            return {};
        }

        return {
            form->GetLocalFormID(),
            (uint32_t)strlen(form->GetFile()->fileName) + 1,
            form->GetFile()->fileName
        };
    }

    RE::FormID SkillSlot::FormIDWithLocalConfiguration(SkillSlotLocalFormConfiguration config) {
        if (!config.localID || !config.pluginName || !config.pluginName) {
            return 0;
        }

        // Lookup the form by the local ID and plugin name
        auto *dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            return 0;
        }

        auto form = dataHandler->LookupForm(config.localID, config.pluginName);
        if (!form) {
            return 0;
        }

        return form->GetFormID();
    }

    SkillSlotLocalFormConfiguration SkillSlot::GetLocalFormConfiguration() {
        if (!_configuration.skillID) {
            return {};
        }

        return LocalConfigurationWithFormID(_configuration.skillID);
    }

    void SkillSlot::SetLocalFormConfiguration(SkillSlotLocalFormConfiguration config) {
        auto formID = FormIDWithLocalConfiguration(config);
        if (!formID) {
            return;
        }

        // Update the configuration with the skill if it was found
        auto skillConfig = _configuration;
        skillConfig.skillID = formID;
        SetConfiguration(skillConfig);
    }

    void SkillSlot::ResetState() {
        _castQueueRetains = 0;
        _castStart = -1.0f;

        _isPressed = false;
        _pressing = false;
        _pressedStart = -1.0f;
        _pressedScale = 1.0f;

        _assignmentProgress = -1.0f;
    }

    void SkillSlot::UpdateSkillTexture() {
        bool hasTexture = false;
        if (_configuration.skillID != 0) {
            // If the configuration specifies a spell, determine the package
            // and local ID so that the right icon can be selected for it
            RE::TESForm *item = RE::TESForm::LookupByID(_configuration.skillID);
            if (!item) {
                return;
            }

            _texture = SkillHUD::GetTextureForSkill(_configuration.skillID);

            TextureLog("Assigned skill with texture %d", _texture.textureID);
            hasTexture = true;
        }

        if (!hasTexture) {
            // Otherwise fall back to an empty button texture
            _texture = SkillHUD::GetEmptySkillSlotTexture();
        }
    }

    void SkillSlot::SetKeybindTexture(KeybindTexture texture) {
        _keybindTexture = texture;
    }

    void SkillSlot::Render(float delta, float scaleY, float translateY, float opacity) {
        RenderAtLocation(_position, delta, scaleY, translateY, opacity);
    }

    bool SkillSlot::CanPress() {
        return !_configuration.skillID == 0;
    }

    void SkillSlot::BeginPress() {
        std::lock_guard lock(_pressLock);

        // If this skill slot has no skill assigned to it, don't play the press animation
        if (!CanPress()) {
            return;
        }

        _isPressed = true;
        _pressing = true;
        _pressedStart = 0.0f;

        _pressedScale = 1.0f;
    }

    void SkillSlot::CancelPress() {
        std::lock_guard lock(_pressLock);

        // If this skill slot is not pressed or pressing, there is nothing to cancel
        if (!_isPressed && !_pressing) {
            return;
        }

        _isPressed = false;
        _pressing = false;
        _pressedStart = -1.0f;

        _pressedScale = 1.0f;
    }

    void SkillSlot::FinishPress() {
        const auto &theme = SkillHUD::Theme();
        std::lock_guard lock(_pressLock);
        
        // If this skill slot is not pressed, don't play the depress animation
        if (!_isPressed) {
            return;
        }

        _isPressed = false;
        _pressing = true;
        _pressedStart = 0.0f;

        _pressedScale = theme.SkillSlotPressedScale;
    }

    RenderMetrics SkillSlot::RenderSlotAtLocation(ImVec2 location, float delta, float scaleY, float translateY, float opacity) {
        const auto &theme = SkillHUD::Theme();
        AdvanceAnimations(delta);

        float additionalScale = 1.0f;
        if (_assignmentProgress != -1.0f) {
            if (_assignmentProgress < 0.5f) {
                additionalScale = InterpolatedValue(1.0f, 1.5f, _assignmentProgress * 2);
            }
            else {
                additionalScale = InterpolatedValue(1.5f, 1.0f, (_assignmentProgress - 0.5f) * 2);
            }
        }

        if (_isPressed || _pressing) {
            additionalScale *= _pressedScale;
        }

        auto width = theme.SkillSlotSizeBase * _scale;
        auto scaleDisplacement = ((width * additionalScale) - width) / 2.0f;
        auto height = width * scaleY * additionalScale;
        width *= additionalScale;

        location.x -= scaleDisplacement;
        location.y -= scaleDisplacement - translateY;

        location.y += (width - height) / 2.0f;

        auto bottomRight = ImVec2 { location.x + width, location.y + height };
        auto drawList = ImGui::GetWindowDrawList();
        
        auto color = 0x00FFFFFF | ((uint32_t)(opacity * 255) << 24);

        // Draw the attunement icon texture
        drawList->AddImage(
            _texture.textureID,
            location, bottomRight,
            _texture.topLeft, _texture.bottomRight,
            color
        );

        // If a skill is assigned, ask for its cooldown and draw a rectangle to cover it
        // to track the cooldown
        if (_configuration.skillID) {
            auto cooldown = SkillHUD::SharedHUD()->CooldownForSkill(_configuration.skillID);
            if (cooldown > 0.0f) {
                ImVec2 topLeft = location;
                float heightBase = (bottomRight.y - location.y);
                float cooldownHeight = heightBase * cooldown;
                topLeft.y += heightBase - cooldownHeight;

                float y = topLeft.y;
                topLeft.y = ceilf(topLeft.y);

                auto cooldownColor = ((uint32_t)(opacity * 180) << 24);
                drawList->AddRectFilled(topLeft, bottomRight, cooldownColor);

                // Manually antialias the top edge for a smooth animation
                if (y != topLeft.y) {
                    ImVec2 aaTopLeft = { topLeft.x, topLeft.y - 1.0f };
                    ImVec2 aaBottomRight = { bottomRight.x, topLeft.y };
                    float aaOpacity = topLeft.y - y;
                    drawList->AddRectFilled(aaTopLeft, aaBottomRight, (uint32_t)(opacity * 180 * aaOpacity) << 24);
                }
            }
        }

        // For left or right hand casting source, also draw an indicator for the source
        auto sourceIndicatorTexture = SkillHUD::GetCastingSourceIndicatorTexture(_configuration.castingSource);
        if (sourceIndicatorTexture.textureID) {
            switch (_configuration.castingSource) {
                case RE::MagicSystem::CastingSource::kLeftHand: {
                    ImVec2 sourceBottomRight {
                        location.x + theme.SkillSlotHandIndicatorSizeBase * _scale * additionalScale,
                        location.y + theme.SkillSlotHandIndicatorSizeBase * _scale * scaleY * additionalScale
                    };

                    drawList->AddImage(
                        sourceIndicatorTexture.textureID,
                        location, sourceBottomRight,
                        sourceIndicatorTexture.topLeft, sourceIndicatorTexture.bottomRight,
                        color
                    );
                    break;
                }
                case RE::MagicSystem::CastingSource::kRightHand: {
                    ImVec2 sourceTopLeft {
                        bottomRight.x - theme.SkillSlotHandIndicatorSizeBase * _scale * additionalScale,
                        location.y
                    };

                    ImVec2 sourceBottomRight {
                        bottomRight.x,
                        location.y + theme.SkillSlotHandIndicatorSizeBase * _scale * scaleY * additionalScale
                    };

                    drawList->AddImage(
                        sourceIndicatorTexture.textureID,
                        sourceTopLeft, sourceBottomRight,
                        sourceIndicatorTexture.topLeft, sourceIndicatorTexture.bottomRight,
                        color
                    );
                    break;
                }
            }
        }

        // Then add the button border on top of it
        auto borderTexture = SkillHUD::GetSkillSlotBorderTexture();
        drawList->AddImage(
            borderTexture.textureID,
            location, bottomRight,
            borderTexture.topLeft, borderTexture.bottomRight,
            color
        );

        return { location, bottomRight, { width, height }, additionalScale };
    }
    
    Rect SkillSlot::RenderAtLocation(ImVec2 location, float delta, float scaleY, float translateY, float opacity) {
        const auto &theme = SkillHUD::Theme();
        const auto &result = RenderSlotAtLocation(location, delta, scaleY, translateY, opacity);

        location = result.origin;

        auto drawList = ImGui::GetWindowDrawList();

        // Finally add the keybind if defined
        if (!_keybindTexture.textureID) {
            return { result.origin, result.destination };
        }

        float keybindHeight = theme.SkillSlotKeybindHeightBase * _scale * result.additionalScale;
        float keybindWidth = keybindHeight * _keybindTexture.sizeRatio;
        keybindHeight *= scaleY;

        ImVec2 keybindTopLeft = {
            location.x + result.size.x / 2.0f - keybindWidth / 2.0f,
            location.y + result.size.y - keybindHeight / 2.0f
        };

        ImVec2 keybindBottomRight = {
            keybindTopLeft.x + keybindWidth,
            keybindTopLeft.y + keybindHeight
        };

        uint32_t color = 0x00FFFFFF | ((uint32_t)(opacity * 255) << 24);
        drawList->AddImage(
            _keybindTexture.textureID,
            keybindTopLeft, keybindBottomRight,
            _keybindTexture.topLeft, _keybindTexture.bottomRight,
            color
        );

        return { result.origin, result.destination };
    }

    void SkillSlot::AdvanceAnimations(float delta) {
        const auto &theme = SkillHUD::Theme();
        if (_pressing) {
            _pressedStart += delta;
            auto time = NormalizedTime(_pressedStart, theme.SkillSlotPressedAnimationDuration);
            time = EaseOutQuartWithTime(time);
            time = std::clamp(time, 0.0f, 1.0f);
            if (time == 1.0f) {
                _pressing = false;
                _pressedStart = -1.0f;
            }

            if (_isPressed) {
                _pressedScale = InterpolatedValue(1.0f, theme.SkillSlotPressedScale, time);
            }
            else {
                _pressedScale = InterpolatedValue(theme.SkillSlotPressedScale, 1.0f, time);
            }
        }
    }

}