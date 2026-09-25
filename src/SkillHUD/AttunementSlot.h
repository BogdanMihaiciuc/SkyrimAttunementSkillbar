#pragma once
#include "imgui_internal.h"
#include "SkillHUD.h"

namespace AttunementSkillbar {

    /**
     * Represents the serializable configuration of an attunement slot instance.
     */
    struct AttunementSlotConfiguration {
        /**
         * The index of this attunement slot.
         */
        uint32_t index;

        /**
         * The ID of the texture selected as the icon for this attunement.
         */
        uint32_t textureID { static_cast<uint32_t>(RE::ActorValue::kAlteration) };
    };

    class SkillSlot;
    struct SkillSlotConfiguration;

    /**
     * Represents an attunement that may selected that contains multiple skill slots.
     * Once obtained, an attunement slot instance must never be held on to beyond the
     * current scope.
     */
    class AttunementSlot {
        friend class SkillHUD;
        friend class SkillSlot;
        friend class SkillAssignmentHUD;
        friend class PersistenceController;
        private:

            /**
             * This attunement slot's configuration.
             */
            AttunementSlotConfiguration _configuration;

            /**
             * The texture to draw for this attunement.
             */
            SkillTexture _texture;

            /**
             * Set to true while this attunement's associated button is depressed.
             */
            bool _isPressed { false };

            /**
             * The time in seconds when this slot was last pressed or released.
             * The time corresponds to the current state.
             */
            float _pressedStart { 0.0f };

            /**
             * Set to true if this is the active attunement.
             */
            bool _isActive { false };

            /**
             * The Skill HUD that contains this attunement slot.
             */
            SkillHUD *_skillHUD;

            /**
             * The number of skill slots contained in this attunement slot.
             */
            uint32_t _skillSlotCount;

            /**
             * The skill slots contained in this attunement.
             */
            SkillSlot **_skillSlots;

            /**
             * The texture used to draw the keybind.
             */
            KeybindTexture _keybindTexture;
            
            /**
             * The computed position of this attunement slot, relative to the
             * top-left corner.
             */
            ImVec2 _position { 0.0f, 0.0f };

            /**
             * The computed base scale of this attunement slot.
             */
            float _scale { 1.0f };

            /**
             * Initializes this attunement slot with the specified configuration.
             * This must not be invoked more than once for an attunement slot instance.
             * @param HUD           The skill HUD that contains this attunement slot.
             * @param config        The configuration this attunement slot should use.
             * @param slotConfig    The skill slot configuration for each of this attunement
             *                      slot's skill slots. The default slot configuration is
             *                      used for all skill slots if not specified.
             */
            void SetConfiguration(
                SkillHUD *HUD,
                AttunementSlotConfiguration config,
                SkillSlotConfiguration *slotConfig = nullptr
            );

            /**
             * Updates the number of skill slots in this attunement to the specified count.
             * @param count         The new number of skill slots to use.
             */
            void Resize(uint32_t count);

        protected:

            /**
             * Creates a skill slot instance.
             * @returns         A skill slot.
             */
            virtual SkillSlot *CreateSkillSlot();

            virtual ~AttunementSlot();

        public:

            /**
             * Sets the keybind texture to be drawn under this attunement slot.
             * @param texture       The keybind texture to use.
             */
            void SetKeybindTexture(KeybindTexture texture);

            /**
             * Renders this attunement slot.
             * @param delta         The amount of time passed since the last update.
             * @param scaleY        If specified, an Y scale to apply to this slot for the
             *                      potion flipping animation.
             * @param translateY    The amount of Y translation to apply when drawing this slot.
             * @param opacity       The opacity with which to draw this attunement slot.
             */
            void Render(float delta, float scaleY, float translateY = 0.0f, float opacity = 1.0f);
    };

}