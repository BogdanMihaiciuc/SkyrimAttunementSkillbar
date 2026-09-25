#pragma once
#include "SkillHUD.h"
#include "../Input/InputEventController.h"

namespace AttunementSkillbar {

    class SkillSlot;

    /**
     * The amount of displacement to apply to the skill HUD for the appear and disappear animation.
     */
    const float AssignmentHUDAppearAnimationDisplacement = 128.0f;

    /**
     * The amount of time in seconds it takes for the skill assignment HUD to appear or disappear.
     */
    const float AssignmentHUDAppearAnimationDuration = 0.3f;

    /**
     * The amount of time in seconds it takes for the skill assignment animation to play.
     */
    const float AssignmentHUDAssignmentAnimationDuration = 0.2f;

    /**
     * A class that manages the appearance of a SkillHUD during the skill assignment process.
     */
    class SkillAssignmentHUD {

        private:
        
            /**
             * The shared HUD instance.
             */
            static SkillAssignmentHUD *_sharedHUD;

            /**
             * The layout configuration normally used by the Skill HUD.
             */
            SkillHUDLayoutConfiguration _layoutConfiguration;

            /**
             * The presentation configuration used for showing the skill assignment HUD.
             */
            SkillHUDLayoutConfiguration _presentationConfiguration;

            /**
             * Set to true while the assignment HUD is visible and while the disappearing
             * animation is running.
             */
            bool _open { false };

            /**
             * Set to true while this assignment HUD is in the processing of appearing.
             */
            bool _appearing { false };

            /**
             * The elapsed time since the appearing animation started.
             */
            float _appearTime { -1.0f };

            /**
             * Set to true while this assignment HUD is in the process of disappearing.
             */
            bool _disappearing { false };

            /**
             * The elapsed time since the disappearing animation started.
             */
            float _disappearTime { -1.0f };

            /**
             * Set to true while this assignment HUD is in the process of assigning a skill.
             */
            bool _assigning { false };

            /**
             * The elapsed time since the assigning animation started.
             */
            float _assignTime { -1.0f };

            /**
             * Set to true when the assignment is for the potion slots.
             */
            bool _assigningPotions { false };

            /**
             * The skill slot on which the assignment animation is playing.
             */
            SkillSlot *_assigningSlot { nullptr };

            SkillAssignmentHUD();

        public:

            /**
             * Obtains a reference to the shared assignment HUD.
             */
            static SkillAssignmentHUD *SharedHUD() {
                if (!_sharedHUD) {
                    _sharedHUD = new SkillAssignmentHUD();
                }

                return _sharedHUD;
            }

            /**
             * Tests whether the assignment HUD is currently open.
             * @returns         True if the assignment HUD is open, false otherwise.
             */
            bool IsOpen();

            /**
             * Launches the assignment HUD. Has no effect if the HUD is already open.
             * @param potionAssignment      Used to determine if the assignment is for potions.
             */
            void Open(PotionAssignmentKind potionAssignment = PotionAssignmentKind::None);

            /**
             * Dismisses the assignment HUD. Has no effect if the HUD is already dismissed
             * or in the process of closing.
             */
            void Dismiss();

            /**
             * Renders the skill assignment HUD.
             */
            void Render();

            /**
             * Assigns the specified skill to the specified slot index in the currently
             * active attunement, then dismisses the assignment HUD. Has no effect if the
             * assignment HUD is not open or in the process of closing.
             * @param skill         The skill to assign.
             * @param potion        If this assignment is for a potion, additional information
             *                      about the potion to assign.
             * @param index         The index of the skill to assign the skill to.
             */
            void AssignSkill(SkillSlotSkillConfiguration skill, PotionSlotConfiguration potion, uint32_t index);

    };

}