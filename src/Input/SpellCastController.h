#pragma once
#include "EquippedItem.h"

namespace AttunementSkillbar {

    struct PotionSlotConfiguration;

    /**
     * Contains constants which describe what action a skill is performed with.
     */
    enum class SkillCastSource {

        /**
         * Indicates that the skill is cast with the left hand.
         */
        LeftHand = 1,

        /**
         * Indicates that the skill is cast with the right hand.
         */
        RightHand = 2,

        /**
         * Indicates that the skill is cast with both hands.
         */
        BothHands = 3,

        /**
         * Indicates that the skill is cast via a power.
         */
        Power = 4,

        /**
         * Indicates that the skill is cast via a power.
         */
        Shout = 5
    };


    /**
     * Constants describing the current state for the controller when waiting
     * for an animation event to occur.
     */
    enum class AnimationWaitingState {

        /**
         * Indicates that the controller is not waiting for the event.
         */
        None = 0,

        /**
         * Indicates that the controller is waiting for the event.
         */
        Waiting = 2,

        /**
         * Indicates that the event has occurred and the queued skill should
         * be performed on the next frame.
         */
        Ready = 3
    };

    /**
     * Describes a spell cast request that is queued or in progress.
     */
    struct SkillCastRequest {

        /**
         * Set to true while the casting key is pressed for this request.
         */
        bool casting { false };

        /**
         * Set to true when an in-progress cast has actually started.
         */
        bool castingStarted { false };

        /**
         * A flag that is set to true when the spell must be released.
         */
        bool release { false };

        /**
         * Set to true if the spell must be manually released.
         */
        bool manuallyReleased { false };

        /**
         * Set to true if this spell cast is waiting for weapons to become ready.
         */
        bool waitingReadyWeapons { false };

        /**
         * Set to true if this spell cast is waiting for weapons to become disabled
         * due to a pending equip event.
         */
        AnimationWaitingState waitingDisableBumper { AnimationWaitingState::None };

        /**
         * Set to true if this spell cast is in the aftercast delay period.
         */
        AnimationWaitingState aftercastDelayFinished { AnimationWaitingState::None };

        /**
         * For spells, the casting source with which to cast.
         */
        SkillCastSource castingSource { SkillCastSource::LeftHand };

        /**
         * The engine time when spell casting actually started
         * for this cast request.
         */
        float holdStart { 0.0f };

        /**
         * The amount of engine time the virtual key must be
         * pressed before the spell is charged.
         */
        float chargeTime { 0.0f };

        /**
         * The engine time when the associated key was pressed for
         * this cast request.
         */
        float keypressStart { 0.0f };

        /**
         * Set to true after the key press has started.
         */
        bool keypressStarted { false };

        /**
         * When set to false, this spell request is unitialized or finalized
         * and should not be used.
         */
        bool initialized { false };

        /**
         * Set to `true` when the spell has finalized, after firing and waiting
         * for the aftercast delay, or immediately upon being interrupted.
         */
        bool finalized { false };

        /**
         * Set to true if this is a power or shout.
         */
        bool isPower { false };

        /**
         * The spell to cast.
         */
        RE::SpellItem *spell { nullptr };

        /**
         * The shout to cast.
         */
        RE::TESShout *shout { nullptr };

        /**
         * The key through which this request was created.
         */
        uint32_t key { 0 };

        /**
         * Used to determine whether the key that created this request
         * is still being held.
         */
        bool keyHeld { false };

        /**
         * Set to true if this request requires a scroll.
         */
        bool isScroll { false };

        /**
         * An optional later start time to begin skill casting at, instead of immediately.
         */
        float startTime { 0.0f };

    };

    /**
     * Describes a spell that has succesfully fired.
     */
    struct FulfilledSpell {

        /**
         * The spell that was fulfilled.
         */
        RE::SpellItem *spell { nullptr };

        /**
         * The engine time at which the spell was fulfilled.
         */
        float finishTime { 0.0f };

        /**
         * The casting source used for the spell.
         */
        SkillCastSource source { SkillCastSource::Power };

    };

    /**
     * Contains constants which describe the action to take when selecting a
     * skill in the skillbar.
     */
    enum class SkillCastBehaviour {

        /**
         * Indicates that the selected skill is only equipped.
         */
        Equip = 1,

        /**
         * Indicates that the skill is equipped and casted.
         */
        Cast = 2,

        /**
         * Indicates that the skill is equipped, casted and then the previously
         * equipped items are equipped back.
         */
        CastAndReequip = 3
    };

    /**
     * Contains constants which describe the action to take when selecting a
     * potion in the potion quickbar.
     */
    enum class PotionSelectBehaviour {

        /**
         * Indicates that the potion is only equipped in the quick slot.
         */
        Equip = 1,

        /**
         * Indicates that the potion is used and then also equipped in the quick slot.
         */
        UseAndEquip = 2
    };

    /**
     * Contains the configuration which controls how selecting skill slots in the
     * skillbar behaves at runtime.
     */
    struct SpellCastControllerConfiguration {

        /**
         * Defines the casting behaviour when selecting skills in the skillbar.
         */
        SkillCastBehaviour castBehaviour { SkillCastBehaviour::CastAndReequip };

        /**
         * Defines the casting behaviour when selecting skills in the skillbar.
         */
        PotionSelectBehaviour potionBehaviour { PotionSelectBehaviour::UseAndEquip };

        /**
         * When enabled, the standard attack/block action for is replaced with an automated
         * cast if the player has a non-concentration spell equipped or has a stored equipment
         * of type non-concentration spell.
         */
        bool autoCastsEquippedSpell { true };

        /**
         * When enabled and autoCastsEquippedSpell is also enabled, if the requested spell
         * is equipped in both hands and can be dual casted, either attack action should
         * perform a dual cast.
         */
        bool prefersDualCast { true };

    };


    /**
     * A struct describing optional overrides for spells that must be used in place of
     * declared spell attributes.
     */
    struct SpellOverrides {

        /**
         * When set to true this override's manually released value must be used.
         */
        bool overridesManuallyReleased { false };

        /**
         * Whether the spell is manually released.
         */
        bool manuallyReleased { true };

    };

    /**
     * A controller that makes it possible to programatically cast an equipped spell
     * by simulating input events and sending them to the attackBlockHandler.
     * 
     * Because there doesn't seem to be an easy way to either start or determine the
     * progress of a spell cast without using the simulated key presses, an animation 
     * listener is additionally used to determine when the spell actually starts casting
     * (it rarely starts as soon as the key down event is dispatched) and then waits for
     * the spell's declared charge time before sending a simulated key up event to release
     * the cast.
     * 
     * Channeled spells keep the simulated key pressed until the stop cast method is
     * manually invoked.
     * 
     * Additionally, if the spell is sheathed, the animation listener is used to determine
     * when the hands are ready for casting.
     */
    class SpellCastController :
        public RE::BSTEventSink<RE::BSAnimationGraphEvent>,
        public RE::BSTEventSink<RE::TESEquipEvent>,
        public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>
    {
        friend class PersistenceController;
        private:

            /**
             * A map that holds optional overrides for spell attributes.
             */
            static std::unordered_map<RE::FormID, SpellOverrides> _spellOverrideMap;

            /**
             * The configuration struct specifying various behaviours for skill casting.
             */
            SpellCastControllerConfiguration _configuration {};

            /**
             * The next queued spell, to be performed after the current action ends.
             */
            SkillCastRequest _queuedSpell {};

            /**
             * The current, in-progress spell cast.
             */
            SkillCastRequest _currentSpell {};

            /**
             * The most recently fulfilled spell.
             */
            FulfilledSpell _fulfilledSpell {};

            /**
             * A mutex used to guard concurrent access to the request key pressed state.
             */
            std::mutex _keyLock;

            /**
             * The player's equipped items before a cast starts.
             */
            StoredEquipment *_storedEquipment { nullptr };

            /**
             * A mutex used to guard concurrent access to the stored equipment.
             */
            std::mutex _storedEquipmentLock;

            /**
             * Set to true while the controller is waiting for the draw animation
             * to finish after re-equipping original equipment. This avoids an issue
             * where casting with the left hand while the weapon draw animation
             * is playing causes it to replay.
             */
            AnimationWaitingState _waitingDrawWeapons { AnimationWaitingState::None };

            /**
             * A mutex used to guard concurrent access to the weapon drawing waiting state.
             */
            std::mutex _drawWeaponLock;

            /**
             * The engine time when the draw weapons waiting started.
             */
            float _drawWeaponsStartTime { 0.0f };

            /**
             * The shared controller.
             */
            static SpellCastController *sharedController;
            
            /**
             * Tests whether the specified player can currently dual cast the specified spell.
             * @param player        The player character.
             * @param spell         The spell to test.
             * @returns             True if the spell can be dual cast, false otherwise.
             */
            bool CanDualCast(RE::PlayerCharacter* player, RE::SpellItem* spell);
            
            /**
             * Tests whether the specified player has the required spell for the request, either
             * known by the player or directly equipped (e.g. a temporary spell a mod forcefully
             * equipes that the player doesn't normally know).
             * @param player        The player character.
             * @param request       The spell request to test.
             * @returns             True if the spell can be cast, false otherwise.
             */
            bool HasSpell(RE::PlayerCharacter* player, const SkillCastRequest &request);

            /**
             * Obtains the charge time of the specified spell.
             * @param spell         The spell whose charge time should be obtained.
             * @returns             The spell's charge time.
             */
            float GetChargeTime(RE::SpellItem *spell);

            
            #ifdef SpellCastMainLoopHook

                /**
                 * Invoked on every main loop frame to advance spell casting and update UI.
                 */
                static void MainLoop();

                /**
                 * The base main loop function that must be invoked for the game.
                 */
                static inline REL::Relocation<decltype(MainLoop)> _baseMainLoop;

            #endif

            /**
             * The overridden animation processor function.
             */
            static RE::BSEventNotifyControl ProcessAnimationEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent> *sink, RE::BSAnimationGraphEvent *event, RE::BSTEventSource<RE::BSAnimationGraphEvent> *source);
            
            /**
             * The base animation event processor function that must be invoked for the game.
             */
            static inline REL::Relocation<decltype(ProcessAnimationEvent)> _baseProcessAnimationEvent;

            /**
             * Set to true while a spell cast controller event is being processed and should not be ignored.
             */
            bool _isControllerEvent { false };

            /**
             * Set to true while the controller is changing equipment to prevent equipment events from
             * cancelling casts.
             */
            bool _isControllerEquipEvent { false };

            /**
             * Set to true when the current cast must be cancelled on the next animation frame.
             */
            bool _cancelCastQueued { false };

            /**
             * The overridden ProcessButton function used to convert regular attack commands into
             * queued spells.
             */
            static void ProcessButton(RE::AttackBlockHandler *obj, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data);

            /**
             * The base ProcessButton function that must be invoked to process attack commands.
             */
            static inline REL::Relocation<decltype(ProcessButton)> _baseProcessButton;

            #ifdef AnimationSkips
            
                /**
                 * Used to skip the spell swap animation.
                 */
                static void UpdateAnimation(RE::Actor* a_this, float a_delta);

                /**
                 * Base update animation function.
                 */
                static inline REL::Relocation<decltype(UpdateAnimation)> _baseUpdateAnimation;
                
                /**
                 * Controls whether animation is skipped for the next equip event. When greater
                 * than 0, this indicates the number of animation steps to skipp.
                 */
                static uint32_t skipsAnimationStep;

            #endif

            /**
             * Set to true once the animation listener has been registered.
             */
            bool _animationListenerRegistered { false };

            /**
             * Starts casting the queued spell, if any. The current spell must not be casting
             * when this method is invoked.
             * @returns         True if a spell is being cast, false otherwise.
             */
            bool CastQueuedSpell();

            /**
             * Queues the specified skill cast request to be cast when the current cast request
             * finishes if it's in progress, or casts it instantly if the player is idle.
             * @param request               The request describing the skill to cast.
             * @param skipsEquipmentSave    When set to true, the currently equipped items
             *                              won't be saved to be restored later.
             */
            void QueueRequest(SkillCastRequest request, bool skipsEquipmentStave = true);

            /**
             * Starts performing the specified skill cast request.
             * @param request               The request describing the skill to cast.
             * @param skipsEquipmentSave    When set to true, the currently equipped items
             *                              won't be saved to be restored later.
             */
            void FulfillRequest(SkillCastRequest request, bool skipsEquipmentStave = true);

            /**
             * Starts performing the specified power or shout cast request.
             * @param request               The request describing the skill to cast.
             * @param skipsEquipmentSave    When set to true, the currently equipped items
             *                              won't be saved to be restored later.
             */
            void FulfillPower(SkillCastRequest request, bool skipsEquipmentStave = true);

            /**
             * Performs the specified skill cast request that represents an instantly activated power.
             * @param request           The request describing the power to use.
             */
            void CastInstantPower(SkillCastRequest request);

            /**
             * Starts performing the specified spell cast request.
             * @param request               The request describing the spell to cast and the
             *                              hand to cast it with.
             * @param skipsEquipmentSave    When set to true, the currently equipped items
             *                              won't be saved to be restored later.
             */
            void FulfillSpell(SkillCastRequest request, bool skipsEquipmentStave = true);
            

            /**
             * Obtains the equipped spell for the specified casting source on the player, if any.
             * @param player        The player whose spell should be obtained.
             * @param source        The casting source from which to obtain the spell.
             * @returns             The equipped spell, if any.
             */
            RE::SpellItem *GetEquippedSpell(RE::PlayerCharacter *player, RE::MagicSystem::CastingSource source);

            /**
             * Obtains the actual casting source that will be used for the specified request.
             * This is typically the same as the one specified in the request, except for when
             * dual cast is request but a dual cast can't be performed; in that case the request
             * will use the right hand casting source.
             * @param request           The request.
             * @returns                 The casting source that will be used for the request.
             */
            SkillCastSource GetRequestActualCastingSource(SkillCastRequest request);

            /**
             * Tests whether the specified perk represents the dual cast perk for the specified
             * magic school.
             * @param perk          The perk to test.
             * @param school        The magic school.
             * @returns             True if the perk represents the dual cast perk for the school,
             *                      false otherwise. 
             */
            static bool IsDualCastPerk(RE::BGSPerk *perk, RE::ActorValue school);

            /**
             * Obtains the user event corresponding to the specified casting source. Non-left/right
             * casting sources will return the event corresponding to dual attack.
             * @param source        The magic source.
             * @returns             The user event.
             */
            static const RE::BSFixedString &AttackUserEvent(SkillCastSource source);

            /**
             * Dispatches a virtual attack/block event corresponding to the specified casting source.
             * This requires the appropriate spell to have been pre-equipped and the player to
             * have drawn their weapon.
             * @param source        The magic source to cast with.
             * @param state         1.0f to start charging, 0.0f to release.
             * @param heldDownSecs  The amount of seconds that charging took.
             */
            inline void SendAttackBlockButton(SkillCastSource source, float state, float heldDownSecs);

            /**
             * Ends the current in-progress cast. Has no effect if there is no in progress
             * cast. If the spell has not fully charged up when this method is called, it
             * will be interrupted.
             */
            void FinishCasting();

            /**
             * Restores the previously saved equipment, if any.
             * @returns         True if the equipment change, false otherwise.
             */
            bool RestoreEquipment();

            /**
             * Plays the spell fire sound effect that is normally cut off when swapping away from
             * the spell that was cast.
             * @param leftHandChanged           Whether the left hand object changed.
             * @param rightHandChanged          Whether the right hand object changed.
             */
            void PlaySpellFireSoundIfNeeded(bool leftHandChanged, bool rightHandChanged);

            /**
             * Plays the spell fire sound effect of the specified spell.
             * @param spell         The spell whose fire sound effect should play.
             */
            static void PlaySpellFireSound(RE::SpellItem *spell);

            /**
             * Tests whether the specified spell is casted from the power slot.
             * @param spell     The spell to test.
             * @returns         True if the spell is a power slot skill, false if it requires
             *                  casting by equipping with hands.
             */
            static bool IsPowerSpell(RE::SpellItem *spell);

        public:

            /**
             * Adds the specified spell attribute overrides.
             */
            static void AddSpellOverride(RE::FormID formID, SpellOverrides override);

            /**
             * Must be invoked as soon as the plugin is initialized to install the main loop
             * hook that is responsible for determining when an in-progress spell should be
             * released and an animation hook that is reponsible for determining when an
             * in-progress spell actually starts charging.
             */
            static void InstallHooks();

            /**
             * Returns the shared controller instance.
             */
            static SpellCastController *SharedController();

            /**
             * Plays a sound to indicate that spell casting failed.
             * @param player        The player.
             * @param spellType     The kind of spell that failed.
             */
            static void PlayMagicFailSound(RE::PlayerCharacter *player, RE::MagicSystem::SpellType spellType);

            /**
             * Causes the magicka bar to flash to indicate the player doesn't have
             * enough magicka to fullfill a spell request.
             */
            static void FlashMagickaBar();

            /**
             * Causes the specified meter to flash the in the HUD menu.
             */
            static void FlashMeter(RE::ActorValue actorValue);

            /**
             * Obtains the equipment slot corresponding to the left hand.
             * @returns         The left hand equipment slot.
             */
            static RE::BGSEquipSlot *GetLeftHandSlot();

            /**
             * Obtains the equipment slot corresponding to the right hand.
             * @returns         The right hand equipment slot.
             */
            static RE::BGSEquipSlot *GetRightHandSlot();

            /**
             * Obtains the equipment slot corresponding to the voice/power skill.
             * @returns         The voice equipment slot.
             */
            static RE::BGSEquipSlot *GetVoiceSlot();

            /**
             * Queues the specified skill ID to be casted with the specified hand, if applicable.
             * @param skillID           The ID of the skill to cast.
             * @param hand              The hand to cast the skill with, if applicable.
             * @param key               The key through which this skill was activated.
             */
            void QueueSkill(RE::FormID skillID, RE::MagicSystem::CastingSource hand, uint32_t key);

            /**
             * When invoked, cancels the current in progress skill cast and cancels any queued
             * skill cast if no hand is specified or auto casting is disabled.
             * If a hand is specified the player normally has a spell equipped in that hand, queues
             * a spell cast with that hand.
             * @param hand                  The hand with which to potentially queue casting.
             * @param key                   The ID of the key that triggered this action.
             * @param restoreEquipment      When set to true, the stored equipment will also be
             *                              restored, otherwise the currently equippet items will be retained.
             */
            void CancelOrQueueCasting(unsigned hand, uint32_t key, bool restoreEquipment = true);

            /**
             * When invoked, cancels the current in progress skill cast and cancels any queued
             * skill cast.
             * @param restoreEquipment      When set to true, the stored equipment will also be
             *                              restored, otherwise the currently equippet items will be retained.
             */
            void CancelCasting(bool restoreEquipment = true);

            /**
             * Should be invoked whenever any key for the active or queued skill cast request is pressed.
             * @param key           The key that was pressed.
             */
            void KeyPressed(uint32_t key);

            /**
             * Should be invoked whenever any key for the active or queued skill cast request is released.
             * @param key           The key that was released.
             */
            void KeyReleased(uint32_t key);

            /**
             * Causes the player to use the specified consumable.
             * @param formID        If the form ID is not set to 0, looks up this specific
             *                      potion or scroll to consume.
             * @param hand          The hand to cast the scroll with. Unused for potions.
             * @param potion        If formID is 0, this describes the kind of potion to
             *                      consume. The potion with the highest magnitude for
             *                      this effect will be used.
             */
            void UseConsumable(RE::FormID formID, RE::MagicSystem::CastingSource hand, PotionSlotConfiguration potion);

            /**
             * Causes the player to consume the specified potion.
             * @param formID        If not 0, looks up this specific potion to consume.
             * @param potion        If formID is 0, this describes the kind of potion to
             *                      consume. The potion with the highest magnitude for
             *                      this effect will be used.
             */
            void ConsumePotion(RE::FormID formID, PotionSlotConfiguration potion);

            /**
             * Causes the player to cast the specified scroll.
             * @param formID            The ID of the scroll to cast.
             * @param hand              The hand to cast the scroll with.
             */
            void CastScroll(RE::FormID formID, RE::MagicSystem::CastingSource hand);
        
            // Invoked to process frame events
            void ProcessFrame();

            // Invoked to determine when spell casting actually starts
            RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent *event, RE::BSTEventSource<RE::BSAnimationGraphEvent> *) override;
            RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent *event, RE::BSTEventSource<RE::TESEquipEvent> *) override;
            RE::BSEventNotifyControl ProcessEvent(const RE::TESSwitchRaceCompleteEvent *event, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent> *) override;

            /**
             * Returns true if attack queuing is enabled and regular attack commands should be ignored.
             */
            bool BlocksAttackEvents();

            // MARK: Configuration

            /**
             * Resets the state of this spell cast controller in preparation for the
             * current session ending.
             */
            void ResetState();

            /**
             * Gets the current skill cast behaviour.
             * @returns         The current skill cast behaviour.
             */
            SkillCastBehaviour GetSkillCastBehaviour();

            /**
             * Sets the skill cast behaviour.
             * @param behaviour     The skill cast behaviour to set.
             */
            void SetSkillCastBehaviour(SkillCastBehaviour behaviour);

            /**
             * Gets the current potion select behaviour.
             * @returns             The current potion select behaviour.
             */
            PotionSelectBehaviour GetPotionSelectBehaviour();

            /**
             * Sets the potion select behaviour.
             * @param behaviour     The potion select behaviour to set.
             */
            void SetPotionSelectBehaviour(PotionSelectBehaviour behaviour);

            /**
             * Gets whether the equipped spell should be auto-cast.
             * @returns             True if auto-casting of the equipped spell is enabled,
             *                      otherwise false.
             */
            bool GetAutoCastsEquippedSpell();

            /**
             * Sets whether the equipped spell should be auto-cast.
             * @param enabled       True to enable auto-casting, false to disable.
             */
            void SetAutoCastsEquippedSpell(bool enabled);

            /**
             * Gets whether the dual cast preference for equipped spells is enabled.
             * @returns     True if dual cast is preferred when the spell is equipped in both 
             *              hands, otherwise false.
             */
            bool GetPrefersDualCast();

            /**
             * Sets the dual cast preference for equipped spells.
             * @param enabled           True to prefer dual casting, false to disable.
             */
            void SetPrefersDualCast(bool enabled);

    };
}