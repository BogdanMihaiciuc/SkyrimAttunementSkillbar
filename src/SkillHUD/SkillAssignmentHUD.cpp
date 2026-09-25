#include "SkillAssignmentHUD.h"
#include "AttunementSlot.h"
#include "SkillSlot.h"
#include "PotionSlot.h"
#include "PotionAttunement.h"
#include "../Input/InputEventController.h"
#include "../Renderer/Renderer.h"

namespace AttunementSkillbar {

    SkillAssignmentHUD::SkillAssignmentHUD() {
        // Setup the default presentation configuration
        _presentationConfiguration = {};
        _presentationConfiguration.anchorPoint = ImVec2(0.0f, 64.0f);
        _presentationConfiguration.sizeScale = 2.0f;
        _presentationConfiguration.horizontalAnchor = SkillHUDConfigurationAnchor::Center;
        _presentationConfiguration.verticalAnchor = SkillHUDConfigurationAnchor::Center;
        _presentationConfiguration.marginLeft = 32.0f;
        _presentationConfiguration.marginRight = 32.0f;
    }

    void SkillAssignmentHUD::Open(PotionAssignmentKind potionAssignment) {
        if (_open) {
            return;
        }

        _layoutConfiguration = SkillHUD::SharedHUD()->_configuration;
        SkillHUD::SharedHUD()->SetLayoutConfiguration(_presentationConfiguration);

        _open = true;
        _appearing = true;
        _disappearing = false;
        _assigning = false;
        _assigningPotions = potionAssignment != PotionAssignmentKind::None;

        if (_assigningPotions) {
            SkillHUD::SharedHUD()->SetShowsPotions(true, false);
        }
        else {
            SkillHUD::SharedHUD()->SetShowsPotions(false, false);
        }
    }

    bool SkillAssignmentHUD::IsOpen() {
        return _open;
    }

    void SkillAssignmentHUD::Dismiss() {
        if (!_open || _disappearing || _assigning) {
            return;
        }
        
        _appearing = false;
        _appearTime = -1.0f;

        _assigning = false;
        _assignTime = -1.0f;

        _disappearing = true;
        _disappearTime = -1.0f;

        _assigningSlot = nullptr;
    }

    void SkillAssignmentHUD::Render() {
        const auto &theme = SkillHUD::Theme();
        static const char *message = "Press the assigned key of the skill slot to assign to, or Esc to cancel";

        auto IO = ImGui::GetIO();
        auto delta = IO.DeltaTime;

        auto list = ImGui::GetWindowDrawList();
        auto scale = std::min(Renderer::GetResolutionScaleHeight(), Renderer::GetResolutionScaleWidth());

        float textAlpha = 1.0f;
        // Position the text center aligned, at an 128pt distance from the skillbar
        ImVec2 textPosition = { IO.DisplaySize.x / 2.0f, IO.DisplaySize.y / 2.0f - 128.0f * scale};

        // Apply the animation offsets, if any animation is running
        if (_appearing) {
            if (_appearTime < 0.0f) {
                _appearTime = 0;
            }

            // Determine the point at which the appearing animation is if it is running
            float normalizedTime = NormalizedTime(_appearTime, theme.AttunementChangeAnimationDuration);
            normalizedTime = EaseOutQuartWithTime(normalizedTime);

            // If the animation ended, reset the animation arguments
            if (normalizedTime >= 1) {
                normalizedTime = 1;
                _appearTime = -1.0f;
                _appearing = false;
            }

            textAlpha = InterpolatedValue(0.0f, 1.0f, normalizedTime);
            textPosition.y = InterpolatedValue(textPosition.y - AssignmentHUDAppearAnimationDisplacement * scale, textPosition.y, normalizedTime);
            SkillHUD::SharedHUD()->_translateY = InterpolatedValue(AssignmentHUDAppearAnimationDisplacement * scale, 0.0f, normalizedTime);
            SkillHUD::SharedHUD()->_opacity = InterpolatedValue(0.0f, 1.0f, normalizedTime);

            _appearTime += delta;
        }
        else if (_disappearing) {
            if (_disappearTime < 0.0f) {
                _disappearTime = 0;
            }

            // Determine the point at which the disappearing animation is if it is running
            float normalizedTime = NormalizedTime(_disappearTime, theme.AttunementChangeAnimationDuration);
            normalizedTime = EaseInQuartWithTime(normalizedTime);

            // If the animation ended, reset the animation arguments
            if (normalizedTime >= 1) {
                normalizedTime = 1;
                _disappearTime = -1.0f;
                _disappearing = false;

                // Once the disappear animation is complete, restore the HUD's normal configuration
                // and resume regular input events
                _appearing = false;
                _open = false;
                SkillHUD::SharedHUD()->_opacity = 1.0f;
                SkillHUD::SharedHUD()->_translateY = 0.0f;
                SkillHUD::SharedHUD()->SetLayoutConfiguration(_layoutConfiguration);
                SkillHUD::SharedHUD()->SetShowsPotions(false, false);
                InputEventController::SharedController()->SkillAssignmentFinished();
                return;
            }

            textAlpha = InterpolatedValue(1.0f, 0.0f, normalizedTime);
            textPosition.y = InterpolatedValue(textPosition.y, textPosition.y - AssignmentHUDAppearAnimationDisplacement * scale, normalizedTime);
            SkillHUD::SharedHUD()->_translateY = InterpolatedValue(0.0f, AssignmentHUDAppearAnimationDisplacement * scale, normalizedTime);
            SkillHUD::SharedHUD()->_opacity = InterpolatedValue(1.0f, 0.0f, normalizedTime);

            _disappearTime += delta;
        }
        else if (_assigning) {
            if (_assignTime < 0.0f) {
                _assignTime = 0;
            }

            // Determine the point at which the disappearing animation is if it is running
            float normalizedTime = NormalizedTime(_assignTime, AssignmentHUDAssignmentAnimationDuration);
            normalizedTime = EaseOutQuartWithTime(normalizedTime);
            _assignTime += delta;
            
            // If the animation ended, reset the animation arguments and dismiss the HUD
            if (normalizedTime >= 1) {
                normalizedTime = 1;
                _assignTime = -1.0f;
                _assigning = false;
                if (_assigningSlot) {
                    _assigningSlot->_assignmentProgress = -1.0f;
                }

                Dismiss();
            }
            else {
                if (_assigningSlot) {
                    _assigningSlot->_assignmentProgress = normalizedTime;
                }
            }
        }

        list->AddRectFilled(
            ImVec2(0.0f, 0.0f),
            ImVec2(IO.DisplaySize.x, IO.DisplaySize.y),
            0x00000000 | ((uint32_t)(textAlpha * 0.75f * 255) << 24)
        );

        auto* font = ImGui::GetDefaultFont();
		float sourceFontSize = ImGui::GetFontSize();
        float targetFontSize = 64.0f * scale;
        float fontRatio = targetFontSize / sourceFontSize;
        
		ImVec2 textSize = ImGui::CalcTextSize(message);
		textSize.x *= fontRatio;
		textSize.y *= fontRatio;
        textPosition.x -= textSize.x / 2.0f;
        textPosition.y -= textSize.y / 2.0f;

        list->AddText(
            font,
            targetFontSize,
            textPosition,
            (uint32_t) (0x00FFFFFF | ((uint32_t)(textAlpha * 255) << 24)),
            message,
            nullptr,
            0.0f,
            nullptr
        );

        SkillHUD::SharedHUD()->Render();
    }

    void SkillAssignmentHUD::AssignSkill(SkillSlotSkillConfiguration skill, PotionSlotConfiguration potion, uint32_t index) {
        // Ignore requests sent if closed or an assignment already in progress
        if (!_open || _disappearing || _assigning) {
            return;
        }

        // Perform the configuration update, then start the assignment animation
        auto skillHUD = SkillHUD::SharedHUD();
        if (_assigningPotions) {
            skillHUD->AssignPotion(skill, potion, index);
            _assigningSlot = skillHUD->_potionAttunement->_skillSlots[index];
        }
        else {
            skillHUD->AssignSkill(skill, index);
            _assigningSlot = skillHUD->_attunementSlots[skillHUD->_activeAttunement]->_skillSlots[index];
        }

        _assigning = true;
        _assignTime = -1.0f;

    }

    SkillAssignmentHUD *SkillAssignmentHUD::_sharedHUD = nullptr;

}