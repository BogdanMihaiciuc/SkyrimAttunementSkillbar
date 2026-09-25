#include "PotionSlot.h"

namespace AttunementSkillbar {

    void PotionSlot::SetConfiguration(SkillSlotConfiguration config, PotionSlotConfiguration genericConfig) {
        SkillSlot::SetConfiguration(config);

        _genericConfiguration = genericConfig;

        UpdateSkillTexture();
    }

    SkillSlotLocalFormConfiguration PotionSlot::GetGenericLocalFormConfiguration() {
        if (!_genericConfiguration.effectID) {
            return {};
        }

        return LocalConfigurationWithFormID(_genericConfiguration.effectID);
    }

    void PotionSlot::SetGenericLocalFormConfiguration(SkillSlotLocalFormConfiguration config, SkillSlotLocalFormConfiguration genericConfig) {
        SetLocalFormConfiguration(config);

        auto genericForm = FormIDWithLocalConfiguration(genericConfig);

        if (!genericForm) {
            return;
        }

        auto genericConfiguration = _genericConfiguration;
        genericConfiguration.effectID = genericForm;
        _genericConfiguration = genericConfiguration;
        
        UpdateSkillTexture();
    }

    void PotionSlot::UpdateSkillTexture() {
        bool hasTexture = false;
        switch (_genericConfiguration.kind) {
            case PotionSlotItemKind::None: {
                break;
            }
            case PotionSlotItemKind::AlcoholDrink: {
                if (_configuration.skillID) {
                    _texture = SkillHUD::GetTextureForAttunement(4);
                }
                else {
                    _texture = SkillHUD::GetTextureForAttunement(5);
                }
                hasTexture = true;
                break;
            }
            case PotionSlotItemKind::RawFood: {
                if (_configuration.skillID) {
                    _texture = SkillHUD::GetTextureForAttunement(6);
                }
                else {
                    _texture = SkillHUD::GetTextureForAttunement(7);
                }
                hasTexture = true;
                break;
            }
            case PotionSlotItemKind::CookedFood: {
                if (_configuration.skillID) {
                    _texture = SkillHUD::GetTextureForAttunement(8);
                }
                else {
                    _texture = SkillHUD::GetTextureForAttunement(9);
                }
                hasTexture = true;
                break;
            }
            case PotionSlotItemKind::Scroll: {
                if (_configuration.skillID) {
                    _texture = SkillHUD::GetTextureForSkill(_configuration.skillID);
                    
                    if (!_texture.textureID || _texture == SkillHUD::GetEmptySkillSlotTexture()) {
                        _texture = SkillHUD::GetTextureForAttunement(10);
                    }
                    hasTexture = true;
                }
                else {
                    hasTexture = false;
                }
                break;
            }
            default: { 
                if (_genericConfiguration.effectID != 0) {
                    if (_configuration.skillID == 0) {
                        _texture = SkillHUD::GetTextureForGenericPotion(_genericConfiguration.effectID);

                        if (!_texture.textureID || _texture == SkillHUD::GetEmptySkillSlotTexture()) {
                            _texture = SkillHUD::GetTextureForAttunement(1);
                        }
                    }
                    else {
                        _texture = SkillHUD::GetTextureForAttunement(_genericConfiguration.effectID);

                        if (_texture.textureID || _texture == SkillHUD::GetEmptySkillSlotTexture()) {
                            _texture = SkillHUD::GetTextureForAttunement(0);
                        }
                    }
                    hasTexture = true;
                }
            }
        }

        if (!hasTexture) {
            // Otherwise fall back to an empty button texture
            _texture = SkillHUD::GetEmptySkillSlotTexture();
        }
    }

    bool PotionSlot::CanPress() {
        return _configuration.skillID != 0 || _genericConfiguration.kind != PotionSlotItemKind::None || _genericConfiguration.effect != RE::ActorValue::kNone;
    }

    bool PotionSlotIndicator::CanPress() {
        return true;
    }

    RenderMetrics PotionSlot::RenderSlotAtLocation(ImVec2 location, float delta, float scaleY, float translateY, float opacity) {
        auto metrics = SkillSlot::RenderSlotAtLocation(location, delta, scaleY, translateY, opacity);

        // If anything is assigned to this potion slot, render its count
        if (_configuration.skillID != 0 || _genericConfiguration.effect != RE::ActorValue::kNone || _genericConfiguration.kind != PotionSlotItemKind::None) {
            auto drawList = ImGui::GetWindowDrawList();

            ImVec2 textPosition = { metrics.destination.x - 12 * _scale, metrics.destination.y - 8 * _scale };

            auto* font = ImGui::GetDefaultFont();
            float sourceFontSize = ImGui::GetFontSize();
            float targetFontSize = 18.0f * _scale;
            float fontRatio = targetFontSize / sourceFontSize;
            
            std::string countString = _count < 100 ? std::to_string(_count) : "99+";
            ImVec2 textSize = ImGui::CalcTextSize(countString.c_str());
            textSize.x *= fontRatio;
            textSize.y *= fontRatio;

            textPosition.x -= textSize.x;
            textPosition.y -= textSize.y;

            ImVec2 backgroundPositionTopLeft = {
                textPosition.x - 8 * _scale,
                textPosition.y - 4 * _scale
            };

            ImVec2 backgroundPositionBottomRight = {
                metrics.destination.x - 4 * _scale,
                metrics.destination.y - 4 * _scale
            };

            auto rectHeight = (backgroundPositionBottomRight.y - backgroundPositionTopLeft.y) * scaleY;
            backgroundPositionTopLeft.y = backgroundPositionBottomRight.y - rectHeight;

            // Also draw a semitransparent background behind the counter for easier readabilty
            drawList->AddRectFilled(
                backgroundPositionTopLeft,
                backgroundPositionBottomRight,
                (uint32_t)((uint32_t)(opacity / 2.0f * 255) << 24),
                4 * _scale
            );

            drawList->AddText(
                font,
                targetFontSize,
                textPosition,
                (uint32_t) (0x00FFFFFF | ((uint32_t)(opacity * scaleY * 255) << 24)),
                countString.c_str(),
                nullptr,
                0.0f,
                nullptr
            );
        }

        return metrics;
    }

}