#pragma once
#include "SkillHUD.h"

namespace AttunementSkillbar {

    class SkillSlot;

    /**
     * The amount of displacement to apply to the skill HUD for the appear and disappear animation.
     */
    const float SkillHUDAppearAnimationDisplacement = 48.0f;

    /**
     * The amount of time in seconds it takes for the skill HUD to appear or disappear.
     */
    const float SkillHUDAppearAnimationDuration = 0.2f;

    /**
     * Describes the situations in which the skill HUD should be drawn.
     */
    struct SkillHUDPresentationConfiguration {

        /**
         * When set to true, the skill HUD will be always shown outside of menus.
         */
        bool showsAlways { true };

        /**
         * When set to true, the skill HUD will be shown whenever the player is in combat.
         */
        bool showsInCombat { true };

        /**
         * When set to true, the skill HUD will be shown whenever the player's weapons are drawn.
         */
        bool showsWeaponsDrawn { true };

    };
    
    /**
     * A class that manages the appearance of the skill HUD depending on various combat conditions.
     */
    class SkillHUDPresentationController {
        friend class PersistenceController;
        private:

            /**
             * The presentation configuration based on which to show or hide the HUD.
             */
            SkillHUDPresentationConfiguration _configuration {};

            /**
             * Set to true while the skill HUD is visible and while the disappearing
             * animation is running.
             */
            bool _open { false };

            /**
             * Set to true while this skill HUD is in the processing of appearing.
             */
            bool _appearing { false };

            /**
             * The elapsed time since the appearing animation started.
             */
            float _appearTime { -1.0f };

            /**
             * Set to true while this skill HUD is in the process of disappearing.
             */
            bool _disappearing { false };

            /**
             * The elapsed time since the disappearing animation started.
             */
            float _disappearTime { -1.0f };
        
            /**
             * The shared HUD instance.
             */
            static SkillHUDPresentationController *_sharedController;

            /**
             * Starts the showing animation for the skill HUD.
             */
            inline void Show();

            /**
             * Starts the hiding animation for the skill HUD.
             */
            inline void Hide();

        public:

            /**
             * Obtains a reference to the shared presentation controller.
             */
            static SkillHUDPresentationController *SharedController() {
                if (!_sharedController) {
                    _sharedController = new SkillHUDPresentationController();
                }

                return _sharedController;
            }

            /**
             * Renders the skill HUD.
             */
            void Render();

            // MARK: Configuration

            /**
             * Gets whether the skill HUD should always be shown.
             * @returns             True if the skill HUD is always shown, false otherwise.
             */
            bool GetShowsAlways();

            /**
             * Sets whether the skill HUD should always be shown.
             * @param enabled       True if the skill HUD is always shown, false otherwise.
             */
            void SetShowsAlways(bool enabled);


            /**
             * Gets whether the skill HUD should be shown while in combat.
             * @returns             True if the skill HUD is shown in combat, false otherwise.
             */
            bool GetShowsInCombat();

            /**
             * Sets whether the skill HUD should be shown while in combat.
             * @param enabled       True if the skill HUD is shown in combat, false otherwise.
             */
            void SetShowsInCombat(bool enabled);


            /**
             * Gets whether the skill HUD should be shown while the player's weapons are drawn.
             * @returns             True if the skill HUD is shown the player's weapons are drawn,
             *                      false otherwise.
             */
            bool GetShowsWeaponsDrawn();

            /**
             * Sets whether the skill HUD should be shown while the player's weapons are drawn.
             * @param enabled       True if the skill HUD is shown the player's weapons are drawn,
             *                      false otherwise.
             */
            void SetShowsWeaponsDrawn(bool enabled);

    };

}