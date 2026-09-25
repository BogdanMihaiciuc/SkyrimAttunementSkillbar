#include "SkillHUDPresentationController.h"
#include "../Input/InputEventController.h"
#include "../Renderer/Renderer.h"

namespace AttunementSkillbar {

    void SkillHUDPresentationController::Show() {
        if (_open && !_disappearing) {
            return;
        }

        _open = true;
        _appearing = true;
        _appearTime = -1.0f;
        _disappearing = false;
    }

    void SkillHUDPresentationController::Hide() {
        if (!_open || _disappearing) {
            return;
        }

        _open = true;
        _appearing = false;
        _disappearing = true;
        _disappearTime = -1.0f;
    }

    void SkillHUDPresentationController::Render() {
        auto IO = ImGui::GetIO();
        auto delta = IO.DeltaTime;

        auto scale = std::min(Renderer::GetResolutionScaleHeight(), Renderer::GetResolutionScaleWidth());

        // Determine whether to show the HUD
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return;
        }

        auto state = player->AsActorState();
        if (!state) {
            return;
        }

        bool renders = _configuration.showsAlways;
        if (!renders) {
            renders = renders || (_configuration.showsInCombat && player->IsInCombat());
            renders = renders || (_configuration.showsWeaponsDrawn && state->IsWeaponDrawn());
        }

        // Rendering is always disabled while player controls are disabled or transformed
        if (InputEventController::IsTransformed() || !InputEventController::IsPlayerInteractionEnabled()) {
            renders = false;
        }

        if (!renders) {
            Hide();
        }
        else {
            Show();
        }

        if (!_open) {
            return SkillHUD::SharedHUD()->AdvanceAnimations();
        }
        
        // Apply the animation offsets, if any animation is running
        if (_appearing) {
            if (_appearTime < 0.0f) {
                _appearTime = 0;
            }

            // Determine the point at which the appearing animation is if it is running
            float normalizedTime = NormalizedTime(_appearTime, SkillHUDAppearAnimationDuration);
            normalizedTime = EaseOutQuartWithTime(normalizedTime);

            // If the animation ended, reset the animation arguments
            if (normalizedTime >= 1) {
                normalizedTime = 1;
                _appearTime = -1.0f;
                _appearing = false;
            }

            SkillHUD::SharedHUD()->_translateY = InterpolatedValue(SkillHUDAppearAnimationDisplacement * scale, 0.0f, normalizedTime);
            SkillHUD::SharedHUD()->_opacity = InterpolatedValue(0.0f, 1.0f, normalizedTime);

            _appearTime += delta;
        }
        else if (_disappearing) {
            if (_disappearTime < 0.0f) {
                _disappearTime = 0;
            }

            // Determine the point at which the disappearing animation is if it is running
            float normalizedTime = NormalizedTime(_disappearTime, SkillHUDAppearAnimationDuration);
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
                return SkillHUD::SharedHUD()->AdvanceAnimations();
            }

            SkillHUD::SharedHUD()->_translateY = InterpolatedValue(0.0f, SkillHUDAppearAnimationDisplacement * scale, normalizedTime);
            SkillHUD::SharedHUD()->_opacity = InterpolatedValue(1.0f, 0.0f, normalizedTime);

            _disappearTime += delta;
        }

        SkillHUD::SharedHUD()->Render();
    }

    // MARK: Configuration

    bool SkillHUDPresentationController::GetShowsAlways() {
        return _configuration.showsAlways;
    }

    void SkillHUDPresentationController::SetShowsAlways(bool enabled) {
        _configuration.showsAlways = enabled;
    }

    bool SkillHUDPresentationController::GetShowsInCombat() {
        return _configuration.showsInCombat;
    }

    void SkillHUDPresentationController::SetShowsInCombat(bool enabled) {
        _configuration.showsInCombat = enabled;
    }


    bool SkillHUDPresentationController::GetShowsWeaponsDrawn() {
        return _configuration.showsInCombat;
    }

    void SkillHUDPresentationController::SetShowsWeaponsDrawn(bool enabled) {
        _configuration.showsInCombat = enabled;
    }

    SkillHUDPresentationController *SkillHUDPresentationController::_sharedController = nullptr;

}