#pragma once
#include "imgui_internal.h"
#include "SkillHUD.h"

namespace AttunementSkillbar {

    /**
     * Describes a 2D rectangle in screen coordinates.
     */
    struct Rect {

        /**
         * The top-left position of the rect.
         */
        ImVec2 origin;

        /**
         * The bottom-right position of the rect.
         */
        ImVec2 destination;
    };

    /**
     * Describes calculation results from rendering a skill slot.
     */
    struct RenderMetrics : public Rect {

        /**
         * The size of the rendered element.
         */
        ImVec2 size;

        /**
         * An additional scale that was applied to the rendered element.
         */
        float additionalScale;
    };

    /**
     * Represents the skill assignemnt portion of the skill slot configuration.
     */
    struct SkillSlotSkillConfiguration {
        /**
         * The ID of the spell contained in this skill slot, or 0 if no spell is contained.
         */
        uint32_t skillID { 0 };

        /**
         * The hand with which to cast this spell.
         */
        RE::MagicSystem::CastingSource castingSource { RE::MagicSystem::CastingSource::kOther };
    };

    /**
     * Represents the local form ID and plugin name of a skill that may be assigned to this skill slot.
     */
    struct SkillSlotLocalFormConfiguration {

        /**
         * The local form ID within the plugin.
         */
        uint32_t localID { 0 };

        /**
         * The length of the plugin name.
         */
        uint32_t pluginNameLength { 0 };

        /**
         * The name of the plugin.
         */
        char *pluginName { nullptr };
    };

    /**
     * Represents the serializable configuration of a skill slot instance.
     */
    struct SkillSlotConfiguration : public SkillSlotSkillConfiguration {
        
        /**
         * The index of this skill slot in the parent skillbar.
         */
        uint32_t index { 0 };

    };

    class AttunementSlot;
    struct SkillTexture;

    /**
     * Represents a skill slot that may contain a spell or shout that can be casted.
     * Once obtained, a skill slot instance must never be held on to beyond the
     * current scope.
     */
    class SkillSlot {
        friend class SkillHUD;
        friend class AttunementSlot;
        friend class SkillAssignmentHUD;
        friend class PersistenceController;
        private:

            /**
             * A mutex used to guard concurrent access to the pressed state.
             */
            std::mutex _pressLock;

            /**
             * Set to true while this skill slot's associated button is depressed.
             */
            bool _isPressed { false };

            /**
             * Set to true while the press/depress animation is running for this skill slot.
             */
            bool _pressing { false };

            /**
             * The time in seconds since this slot was last pressed or released.
             * The time corresponds to the current state. A negative value
             * indicates that the animation is complete.
             */
            float _pressedStart { -1.0f };

            /**
             * The additional scale to use when pressing this skill slot.
             */
            float _pressedScale { 1.0f };

            /**
             * The amount of times this slot is currently involved in spell casts or queues.
             * When greater than 0, the casting animation plays for this slot.
             */
            int _castQueueRetains { 0 };

            /**
             * The time elapsed in seconds when `_castQueueRetains` was last increased from 0.
             */
            float _castStart { -1.0f };

            /**
             * The attunement that this skill slot is part of.
             */
            AttunementSlot *_attunementSlot;

            /**
             * The progress of the assignment animation. This is set and managed by
             * the skill assignment HUD.
             */
            float _assignmentProgress { -1.0f };

        protected:
            
            /**
             * The computed position of this skill slot, relative to the
             * top-left corner.
             */
            ImVec2 _position { 0.0f, 0.0f };

            /**
             * The computed base scale of this skill slot.
             */
            float _scale { 1.0f };

            /**
             * The configuration used by this skill slot.
             */
            SkillSlotConfiguration _configuration;

            /**
             * Updates this skill slot's skill texture based on the current configuration.
             */
            virtual void UpdateSkillTexture();

            /**
             * The texture with which to render this skill slot's spell icon.
             */
            SkillTexture _texture;

            /**
             * The texture with which to render this skill slot's keybind.
             */
            KeybindTexture _keybindTexture;

            /**
             * Tests whether this skill slot can play the pressing animation.
             * @returns         True if this skill slot can be pressed, false otherwise.
             */
            virtual bool CanPress();

            /**
             * Creates and returns a local form configuration for the specified form ID.
             * @param formID            The form ID for which to create the configuration.
             * @returns                 The local form configuration.
             */
            SkillSlotLocalFormConfiguration LocalConfigurationWithFormID(RE::FormID formID);

            /**
             * Obtains the form ID associated with the specified local form configuration
             * @param formID            The local form configuration.
             * @returns                 The associated form ID, or `0` if it could not be determined.
             */
            RE::FormID FormIDWithLocalConfiguration(SkillSlotLocalFormConfiguration config);

            /**
             * Renders this skill at the specified location.
             * @param location      The location at which to render this skill slot.
             * @param delta         The amount of time elapsed since the last drawn frame.
             * @param scaleY        If specified, an Y scale to apply to this slot for the
             *                      attunement flipping animation.
             * @param translateY    The amount of Y translation to apply when drawing this slot.
             * @param opacity       The opacity with which to draw this skill slot.
             * @returns             The rect that this skill slot was drawn in.
             */
            virtual RenderMetrics RenderSlotAtLocation(ImVec2 location, float delta, float scaleY = 1.0f, float translateY = 0.0f, float opacity = 1.0f);

            virtual ~SkillSlot() {};

        public:

            /**
             * Reset this skill slot's animation state in preparation for being reused.
             */
            void ResetState();

            /**
             * Renders this skill slot.
             * @param delta         The amount of time elapsed since the last drawn frame.
             * @param scaleY        If specified, an Y scale to apply to this slot for the
             *                      attunement flipping animation.
             * @param translateY    The amount of Y translation to apply when drawing this slot.
             * @param opacity       The opacity with which to draw this skill slot.
             */
            void Render(float delta, float scaleY = 1.0f, float translateY = 0.0f, float opacity = 1.0f);

            /**
             * Renders this skill at the specified location.
             * @param location      The location at which to render this skill slot.
             * @param delta         The amount of time elapsed since the last drawn frame.
             * @param scaleY        If specified, an Y scale to apply to this slot for the
             *                      attunement flipping animation.
             * @param translateY    The amount of Y translation to apply when drawing this slot.
             * @param opacity       The opacity with which to draw this skill slot.
             * @returns             The rect that this skill slot was drawn in.
             */
            virtual Rect RenderAtLocation(ImVec2 location, float delta, float scaleY = 1.0f, float translateY = 0.0f, float opacity = 1.0f);

            /**
             * Sets this skill slot's configuration.
             * @param config        The configuration to use.
             */
            void SetConfiguration(SkillSlotConfiguration config);

            /**
             * Obtains the local form configuration for the current configuration.
             * @returns             The local form configuration.
             */
            SkillSlotLocalFormConfiguration GetLocalFormConfiguration();

            /**
             * Updates the skill slot's skill configuration using the skill specified
             * in the local form configuration.
             * @param config        The local form configuration.
             */
            void SetLocalFormConfiguration(SkillSlotLocalFormConfiguration config);

            /**
             * Sets the keybind texture to be drawn under this skill slot.
             * @param texture       The keybind texture to use.
             */
            void SetKeybindTexture(KeybindTexture texture);

            /**
             * Causes animations to advance without actually rendering anything.
             * @param delta     The amount of time elapsed since the last drawn frame.
             */
            void AdvanceAnimations(float delta);

            /**
             * Starts the press animation for this button.
             */
            void BeginPress();

            /**
             * Cancels the press state for this button, without any animation.
             */
            void CancelPress();

            /**
             * Starts the depress animation for this button.
             */
            void FinishPress();
    };

}