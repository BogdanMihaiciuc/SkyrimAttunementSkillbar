#pragma once
#include "../SkillHUD/SkillSlot.h"
#include "../SkillHUD/PotionSlot.h"

namespace AttunementSkillbar {

    /**
     * The amount of time, in seconds, that the potion key must be held before
     * the potions UI is displayed.
     */
    const float PotionSwapHoldTime = 0.2f;

    /**
     * A keybind to one of the relevant 
     */
    struct Keybind {

        /**
         * The event triggered by this keybind.
         */
        const RE::BSFixedString &event;

        /**
         * The ID of the bound key.
         */
        uint32_t keyID;

        /**
         * The device from which the key was registered.
         */
        RE::INPUT_DEVICE device;

        /**
         * If greater than 0, this keybind represents an attack command for
         * the specified hand.
         */
        unsigned attackHand { 0 };

        /**
         * If set to true, this event causes the stored equipment to be restored,
         * otherwise it is retained.
         */
        bool restoresEquipment { true };

    };

    /**
     * A struct that defines a keybind that applies to a skill slot.
     */
    struct SkillbarKeybind {
        
        /**
         * The ID of the bound key.
         */
        uint32_t keyID;

        bool operator==(const SkillbarKeybind &other) const {
            return keyID == other.keyID;
        }

    };


    /**
     * An object that contains the configuration for the UI portion of
     * the attunement skillbar.
     */
    struct InputEventControllerConfiguration {

        /**
         * The number of available attunements.
         */
        uint32_t attunementCount { 4 };

        /**
         * The number of skill slots for each attunement.
         */
        uint32_t skillSlotCount { 10 };

        /**
         * The optional modifier key to use to bring up the skill assignment menu.
         */
        uint32_t assignmentModifierKeyID { RE::BSKeyboardDevice::Keys::kLeftControl };

        /**
         * The key to use to assign a skill to the right hand.
         */
        uint32_t assignRightHandKeyID { 256 };

        /**
         * The key to use to assign a skill to the left hand.
         */
        uint32_t assignLeftHandKeyID { 257 };

        /**
         * The modifier to key to use to modify an assignment to prefer dual casting.
         */
        uint32_t assignDualCastModifierKeyID { RE::BSKeyboardDevice::Keys::kLeftShift };

        /**
         * The key used to activate or change potions.
         */
        uint32_t potionKeyID { RE::BSKeyboardDevice::Keys::kQ };

        /**
         * The key used to activate the next attunement.
         */
        uint32_t nextAttunementKeyID { 0 };

        /**
         * The key used to activate the previous attunement.
         */
        uint32_t previousAttunementKeyID { 0 };
    };

    /**
     * Constants which describe how a potion should be assigned to a potion slot from the
     * inventory.
     */
    enum class PotionAssignmentKind {

        /**
         * Indicates that potion assignment is not taking place.
         */
        None = 0,

        /**
         * Indicates that a generic potion should be assigned that finds
         * the inventory potion with the highest actor value when consuming.
         */
        Generic = 1,

        /**
         * Indicates that the specifically highlighted potion is assigned.
         */
        Specific = 2,

    };

    /**
     * Listens for input events and invokes spell casting or skillbar assignments
     * if needed.
     */
    class InputEventController final :
        public RE::BSTEventSink<RE::InputEvent *>,
        public RE::BSTEventSink<RE::MenuOpenCloseEvent> 
    {
        friend class SkillAssignmentHUD;
        friend class PersistenceController;
        private:
            /**
             * The shared input event sink.
             */
            static InputEventController *_sharedController;

            /**
             * Set to true while the game is loading.
             */
            static bool _gameLoading;

            /**
             * Used to temporarily disable input events from the game.
             */
            static void DispatchInputEvent(RE::BSTEventSource<RE::InputEvent *> *eventSource, RE::InputEvent **event);

            /**
             * The base input event dispatcher function that must be invoked for the game.
             */
            static inline REL::Relocation<decltype(DispatchInputEvent)> _baseDispatchInputEvent;

            /**
             * The configuration used by the input event controller.
             */
            InputEventControllerConfiguration _configuration {};

            /**
             * The dodge key, if configured.
             */
            uint32_t _dodgeKey { 0 };

            /**
             * The keybind assigned to each attunement slot.
             */
            SkillbarKeybind *_attunementKeybinds { nullptr };

            /**
             * The keybind assigned to each skill slot.
             */
            SkillbarKeybind *_skillKeybinds { nullptr };

            /**
             * Invoked to update the keybinds cache.
             */
            void ReloadKeybinds();

            /**
             * Gets and caches the keybinds for the specified user event.
             * @param event                 The user event.
             * @param hand                  If specified and greater than 0, for an attack event
             *                              this represents the hand the event refers to.
             * @param restoresEquipment     Whether this event also causes the stored equipment
             *                              to be restored.
             */
            void CacheKeybindsForEvent(RE::BSFixedString &event, unsigned hand = 0, bool restoresEquipment = true);

            /**
             * Contains cached keybinds.
             */
            std::vector<Keybind> *keybinds;

            /**
             * Tests whether the specified button event should cancel spell casting.
             * @param button        The button to test.
             * @param hand          When this method returns true, this value will be updated
             *                      to 0 if the event is not an attack event or the hand to
             *                      which the event refers if it is.
             * @param hand          When this method returns true, this value will be updated
             *                      to 0 if the event is not an attack event or the device independent
             *                      key of the event if it is.
             * @param restoresEquip When this method returns true, this value will be updated
             *                      to true if the event should restore the stored equipment or false
             *                      otherwise.
             * @returns             True if the button should cancel spell casting,
             *                      false otherwise.
             */
            bool IsSpellCancellingButton(RE::ButtonEvent *button, unsigned &hand, uint32_t &key, bool &restoresEquip);

            /**
             * Set true while the assignment modifier key is held.
             */
            bool _assignmentModifierHeld { false };

            /**
             * Set to true while the dual cast modifier key is held.
             */
            bool _dualCastModifierHeld { false };

            /**
             * Set to true while the potion key is held.
             */
            bool _potionKeyHeld { false };

            /**
             * Set to true after the active potion slot is changed while the potion key is held.
             * When true, releasing the potion key does not cause the potion in that slot to
             * not be consume.
             */
            bool _potionSwapped { false };

            /**
             * Set to true while the potion HUD is activated because the potion key is held.
             */
            bool _potionHUDActive { false };

            /**
             * Describes the skill or potion that is waiting to be assigned to a skill slot.
             */
            SkillSlotSkillConfiguration _skillToAssign;

            /**
             * Describes additional information about the potion that is waiting to be assigned
             * to a potion slot.
             */
            PotionSlotConfiguration _potionToAssign;

            /**
             * Describes how the potion that is waiting to be assigned should be assigned.
             */
            PotionAssignmentKind _potionAssignmentKind { PotionAssignmentKind::None };

            /**
             * When set to `true`, user input is disabled for the game and is
             * only processed by the input event controller.
             */
            static bool _inputDisabled;

            /**
             * Suspends all interaction with the game until `SkillAssignmentFinished` is invoked.
             * This method has no effect if interaction is already suspended.
             */
            static void ShowSkillAssignmentHUD();

            /**
             * Resumes interaction if it is currently blocked by a `ShowSkillAssignmentHUD` call.
             */
            static void SkillAssignmentFinished();

            /**
             * Retrieves the device-independent ID code from the specified button event, based on
             * the event's native ID code and the device kind, so it matches the key codes that
             * are set via the MCM configuration.
             * @param event         The button event.
             * @returns             The MCM specific ID code.
             */
            static uint32_t GetDeviceIndependentIDCode(RE::ButtonEvent *event);

            /**
             * Retrieves the device-independent ID code from the specified ID code and device kind,
             * so it matches the key codes that are set via the MCM configuration.
             * @param IDCode        The device-specific ID code.
             * @param device        The kind of device.
             * @returns             The MCM specific ID code.
             */
            static uint32_t GetDeviceIndependentIDCode(uint32_t IDCode, RE::INPUT_DEVICE device);

            /**
             * Removes the current skill keybind instances and frees their memory.
             */
            void ReleaseSkillKeybinds();

            /**
             * Returns true if the assignment modifier is currently held or not assigned.
             * @returns     The assignment modifier held state.
             */
            bool IsAssignmentModifierHeld();

            /**
             * Handles a button event that occurs while the magic menu is open.
             * @param event         The button event.
             * @returns             True if the input event controller wants to handle
             *                      this event, false otherwise.
             */
            bool HandleMagicMenuButtonEvent(RE::ButtonEvent *event);

            /**
             * Assigns the currently highlighted (if any) spell to a skill slot to be cast
             * with the specified hand. Has no effect if a configured assignment modifier
             * is not pressed.
             * The magic menu must be open when this method is invoked.
             * @param hand          The hand to cast the spell with, if applicable.
             */
            void BeginSpellAssignmentForHand(RE::MagicSystem::CastingSource hand);

            /**
             * Handles a button event that occurs while the inventory menu is open.
             * @param event         The button event.
             * @returns             True if the input event controller wants to handle
             *                      this event, false otherwise.
             */
            bool HandleInventoryMenuButtonEvent(RE::ButtonEvent *event);
            
            /**
             * Assigns the currently highlighted (if any) potion to a potion slot to be consumed.
             * Has no effect if a configured assignment modifier is not pressed.
             * The inventory menu must be open when this method is invoked.
             * @param kind              The kind of assignment to perform.
             */
            void BeginPotionAssignment(PotionAssignmentKind kind);

            /**
             * Retrieves the attunement slot index that corresponds to the specified event,
             * if it is a button event whose key represents the assigned key of an attunement.
             * @param event         The button event.
             * @returns             The index of the attunement corresponding to the event, or
             *                      -1 if the event corresponds to no attunement.
             */
            int32_t GetAttunementSlotIndexForEvent(RE::ButtonEvent *event);

            /**
             * Retrieves the skill slot index that corresponds to the specified event,
             * if it is a button event whose key represents the assigned key of a skill slot.
             * @param event         The button event.
             * @returns             The index of the skill slot corresponding to the event, or
             *                      -1 if the event corresponds to no skill slot.
             */
            int32_t GetSkillSlotIndexForEvent(RE::ButtonEvent *event);

            /**
             * Tests whether the specified button event refers to the potion key.
             * @param event         The button event.
             * @returns             True if the event is for the potion key, false otherwise.
             */
            bool IsPotionKey(RE::ButtonEvent *event);

            /**
             * Tests whether the specified button event refers to the next attunement key.
             * @param event         The button event.
             * @returns             True if the event is for the next attunement key, false otherwise.
             */
            bool IsNextAttunementKey(RE::ButtonEvent *event);

            /**
             * Tests whether the specified button event refers to the previous attunement key.
             * @param event         The button event.
             * @returns             True if the event is for the previous attunement key, false otherwise.
             */
            bool IsPreviousAttunementKey(RE::ButtonEvent *event);

        public:

            static void InstallHooks();

            static InputEventController *SharedController() {
                if (!_sharedController) {
                    _sharedController = new InputEventController();
                    _sharedController->UseDefaultConfiguration();
                    _sharedController->ReloadKeybinds();
                }

                return _sharedController;
            }

            InputEventController() {
                keybinds = new std::vector<Keybind>();
            }

            ~InputEventController() {
                delete keybinds;
            }

            /**
             * Sets the game loading state.
             * @param loading       True if the game is loading, false otherwise.
             */
            static void SetGameLoading(bool loading);

            /**
             * Tests whether the player character is transformed.
             * @returns         True if the player is transformed, false otherwise.
             */
            static bool IsTransformed();

            /**
             * Tests whether player interaction is currently enabled.
             * @returns         True if player interation is enabled, false otherwise.
             */
            static bool IsPlayerInteractionEnabled();

            /**
             * Creates and returns a potion configuration with the specified scroll item.
             * @param item          The scroll item.
             * @returns             A potion configuration.
             */
            static PotionSlotConfiguration PotionConfigurationWithScrollItem(RE::ScrollItem *item);

            /**
             * Creates and returns a potion configuration with the specified alchemy item.
             * @param item          The alchemy item.
             * @returns             A potion configuration.
             */
            static PotionSlotConfiguration PotionConfigurationWithAlchemyItem(RE::AlchemyItem *item);
            

            /**
             * Returns the magnitude of the hunger effect of the specified food, if any.
             * @param item          The alchemy item whose configuration to obtain.
             * @returns             The magnitude of the hunger efect, or 0.01 if the item does not have
             *                      any matching effect.
             */
            static float HungerMagnitudeForFoodItem(RE::AlchemyItem *item);

            /**
             * Returns the magnitude of the effect matching the specified configuration, if any.
             * @param item          The alchemy item whose configuration to obtain.
             * @param config        The config specifying the generic effect.
             * @returns             The magnitude of the potion, or 0 if the potion does not have
             *                      any matching effect.
             */
            static float PotionMagnitudeForConfiguration(RE::AlchemyItem *item, PotionSlotConfiguration config);

            /**
             * Reads the dodge keybind from TK Dodge if present.
             */
            void InitializeDodgeKey();

            /**
             * Retrieves the cached keybind assigned to the specified event.
             * @param event     The event.
             * @returns         The keybind.
             */
            Keybind GetKeybindForEvent(const RE::BSFixedString &event);

            /**
             * Invoked to process an input event that occurs while the assignment HUD
             * is being displayed.
             * @param event         The input event.
             * @param eventSource   The source of the event.
             */
            RE::BSEventNotifyControl ProcessAssignmentEvent(
                RE::InputEvent *const *event,
                RE::BSTEventSource<RE::InputEvent *> *eventSource
            );

            RE::BSEventNotifyControl ProcessEvent(
                RE::InputEvent *const *event,
                RE::BSTEventSource<RE::InputEvent *> *eventSource
            ) override;

            RE::BSEventNotifyControl ProcessEvent(
                const RE::MenuOpenCloseEvent *event,
                RE::BSTEventSource<RE::MenuOpenCloseEvent> *eventSource
            ) override;

            /**
             * Returns the hand that corresponds to the attack event bound to the specified
             * button.
             * @param button        The button event.
             * @returns             An unsigned int that corresponds to the hand of the associated
             *                      attack event bound to the button, or 0 if no attack action
             *                      is bound to the button.
             */
            unsigned GetHandForButtonEvent(RE::ButtonEvent *button);

            // MARK: Configuration

            /**
             * Resets the state of this input event controller in preparation for the
             * current session ending.
             */
            void ResetState();

            /**
             * Causes this input event controller to use the default configuration.
             */
            void UseDefaultConfiguration();

            /**
             * Updates the configuration of this input event controller with the specified
             * configuration and keybinds.
             * @param configuration         The configuration indicating the number of keybinds.
             * @param attunementKeybinds    An array of skill keybinds corresponding to attunements.
             *                              The caller retains ownership of this object.
             * @param skillKeybinds         An array of skill keybinds corresponding to skill slots.
             *                              The caller retains ownership of this object.
             */
            void SetConfiguration(
                InputEventControllerConfiguration configuration,
                SkillbarKeybind *attunementKeybinds,
                SkillbarKeybind *skillKeybinds
            );

            /**
             * Gets the modifier key ID used to bring up the skill assignment menu.
             * @returns         The assignment modifier key ID.
             */
            uint32_t GetAssignmentModifierKeyID() const;

            /**
             * Sets the modifier key ID used to bring up the skill assignment menu.
             * @param keyID     The new assignment modifier key ID.
             */
            void SetAssignmentModifierKeyID(uint32_t keyID);


            /**
             * Gets the key ID used to assign a skill to the right hand.
             * @returns         The right-hand assignment key ID.
             */
            uint32_t GetAssignRightHandKeyID() const;

            /**
             * Sets the key ID used to assign a skill to the right hand.
             * @param keyID     The new right-hand assignment key ID.
             */
            void SetAssignRightHandKeyID(uint32_t keyID);


            /**
             * Gets the key ID used to assign a skill to the left hand.
             * @returns The left-hand assignment key ID.
             */
            uint32_t GetAssignLeftHandKeyID() const;

            /**
             * Sets the key ID used to assign a skill to the left hand.
             * @param keyID The new left-hand assignment key ID.
             */
            void SetAssignLeftHandKeyID(uint32_t keyID);


            /**
             * Gets the modifier key ID used to modify an assignment to prefer dual casting.
             * @returns The dual-cast modifier key ID.
             */
            uint32_t GetAssignDualCastModifierKeyID() const;

            /**
             * Sets the modifier key ID used to modify an assignment to prefer dual casting.
             * @param keyID The new dual-cast modifier key ID.
             */
            void SetAssignDualCastModifierKeyID(uint32_t keyID);


            /**
             * Gets the key ID used to consume or assign an alchemy item.
             * @returns         The potion key ID.
             */
            uint32_t GetPotionKeyID() const;

            /**
             * Sets the key ID used to consume or assign an alchemy item.
             * @param keyID     The new potion key ID.
             */
            void SetPotionKeyID(uint32_t keyID);


            /**
             * Gets the key ID used to select the next attunement.
             * @returns         The next attunement key ID.
             */
            uint32_t GetNextAttunementKeyID() const;

            /**
             * Sets the key ID used to select the next attunement.
             * @param keyID     The new next attunement key ID.
             */
            void SetNextAttunementKeyID(uint32_t keyID);


            /**
             * Gets the key ID used to select the previous attunement.
             * @returns         The previous attunement key ID.
             */
            uint32_t GetPreviousAttunementKeyID() const;

            /**
             * Sets the key ID used to select the previous attunement.
             * @param keyID     The new previous attunement key ID.
             */
            void SetPreviousAttunementKeyID(uint32_t keyID);

            
            /**
             * Unassigns the specified keybind from all attunement and skill slots using it.
             * @param keybind           The keybind to unassign.
             */
            void UnassignKeybind(SkillbarKeybind keybind);
            
            
            /**
             * Gets the currently configured attunement count.
             */
            uint32_t GetAttunementCount();

            /**
             * Configures the input event controller to use the specified number of attunements. If this
             * is less than the current number of attunements, the extra attunements will be
             * deleted with their keybind assignments.
             * @param count         The new attunement count to use.
             */
            void SetAttunementCount(uint32_t count);

            /**
             * Gets the currently configured skill slot count.
             */
            uint32_t GetSkillCount();

            /**
             * Configures the input event controller to use the specified number of skill slots. If this
             * is less than the current number of skill slots, the extra skill slots will be
             * deleted with their keybind assignments.
             * @param count         The new attunement count to use.
             */
            void SetSkillCount(uint32_t count);
            
            /**
             * Returns the currently assigned keybind for the attunement at the specified index.
             * @param index         The index of the attunement whose keybind should be obtained.
             */
            SkillbarKeybind GetKeybindForAttunement(int32_t index);

            /**
             * Assigns the specified keybind for the attunement at the specified index.
             * @param index         The index of the attunement whose keybind should be obtained.
             * @param keybind       The keybind to use.
             */
            void SetKeybindForAttunement(int32_t index, SkillbarKeybind keybind);

            /**
             * Returns the currently assigned keybind for the skill at the specified index.
             * @param index         The index of the skill whose keybind should be obtained.
             * @returns             The current keybind.
             */
            SkillbarKeybind GetKeybindForSkill(int32_t index);

            /**
             * Assigns the specified keybind for the skill at the specified index.
             * @param index         The index of the skill whose keybind should be obtained.
             * @param keybind       The keybind to use.
             */
            void SetKeybindForSkill(int32_t index, SkillbarKeybind keybind);
    };
}