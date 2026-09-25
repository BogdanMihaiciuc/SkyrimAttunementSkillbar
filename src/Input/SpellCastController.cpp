#include "SpellCastController.h"
#include "InputEventController.h"
#include "../SkillHUD/PotionSlot.h"

// #define RuntimeDebugCast
#ifdef RuntimeDebugCast
#define RuntimeCastLog(...) RE::ConsoleLog::GetSingleton()->Print(__VA_ARGS__)
#else
#define RuntimeCastLog(...)
#endif

// #define AnimationEventsDebug
#ifdef AnimationEventsDebug
#define AnimationLog(...) RE::ConsoleLog::GetSingleton()->Print(__VA_ARGS__)
#else
#define AnimationLog(...)
#endif

// #define EquipmentDebug
#ifdef EquipmentDebug
#define EquipmentLog(...) RE::ConsoleLog::GetSingleton()->Print(__VA_ARGS__)
#else
#define EquipmentLog(...)
#endif

// #define AutocastDebug
#ifdef AutocastDebug
#define AutocastLog(...) RE::ConsoleLog::GetSingleton()->Print(__VA_ARGS__)
#else
#define AutocastLog(...)
#endif

// #define ScrollDebug
#ifdef ScrollDebug
#define ScrollLog(...) RE::ConsoleLog::GetSingleton()->Print(__VA_ARGS__)
#else
#define ScrollLog(...)
#endif

// #define SoundDebug
#ifdef SoundDebug
#define SoundLog(...) RE::ConsoleLog::GetSingleton()->Print(__VA_ARGS__)
#else
#define SoundLog(...)
#endif


namespace AttunementSkillbar {

    /**
     * The amount of time the controller is allowed to wait for weapons drawn,
     * bumper disable, or spell cast start events before timing out.
     */
    const float WaitingTimeout = 1.5f;

    /**
     * The amount of time the controller is allowed to wait for the aftercast
     * delay of the current spell before timing out.
     */
    const float AftercastDelayTimeout = 2.5f;

    /**
     * The amount of time the controller is allowed to wait for the weapons
     * drawn animation before starting a spell.
     */
    const float WeaponDrawTimeout = 3.5f;

    /**
     * The amount of additional aftercast delay to wait after the ritual spell
     * finishes before marking it as complete. This probably won't work in some situations,
     * but the other timeouts should at least prevent automated spell casting to
     * get completely stuck on waiting for the aftercast delay in those cases.
     */
    const float RitualAftercastDelayTimeout = 0.5f;

    /**
     * The amount of additional waiting after unequipping a staff weapon before attempting to
     * start a cast.
     */
    const float StaffStartWindupTimeout = 0.16f;

    /**
     * The interval in which to manually play the spell fire event if changing the equipped
     * item of the spell's casting source.
     */
    const float SpellFireSoundTimeout = 0.05f;
    
    /**
     * A pointer to a float that represents the current engine time.
     */
    REL::Relocation<float*> engineTime { RELOCATION_ID(517597, 404125) };

    SpellCastController *SpellCastController::SharedController() {
        if (!sharedController) {
            sharedController = new SpellCastController();
        }
        return sharedController;
    }

    /**
     * Returns the equivalent skill cast source for the specified magic casting source.
     * @param source        The magic casting source.
     * @returns             The corresponding skill casting source.
     */
    SkillCastSource SkillCastSourceWithMagicSource(RE::MagicSystem::CastingSource source) {
        switch (source) {
            case RE::MagicSystem::CastingSource::kLeftHand:
                return SkillCastSource::LeftHand;
            case RE::MagicSystem::CastingSource::kRightHand:
                return SkillCastSource::RightHand;
            default:
                return SkillCastSource::BothHands;
        }
    }
    
    /**
     * Returns the equivalent magic cast source for the specified skill casting source.
     * @param source        The skill casting source.
     * @returns             The corresponding magic casting source.
     */
    RE::MagicSystem::CastingSource MagicSourceWithSkillCastSource(SkillCastSource source) {
        switch (source) {
            case SkillCastSource::LeftHand:
                return RE::MagicSystem::CastingSource::kLeftHand;
            case SkillCastSource::RightHand:
                return RE::MagicSystem::CastingSource::kRightHand;
            default:
                return RE::MagicSystem::CastingSource::kOther;
        }
    }

    // MARK: Spell stats
    
    RE::SpellItem *SpellCastController::GetEquippedSpell(RE::PlayerCharacter *player, RE::MagicSystem::CastingSource source) {
        if (!player) {
            return nullptr;
        }

        bool leftHand = source == RE::MagicSystem::CastingSource::kLeftHand;
        RE::TESForm *form = player->GetEquippedObject(leftHand);
        return form && form->GetFormType() == RE::FormType::Spell ? form->As<RE::SpellItem>() : nullptr;
    }

    bool SpellCastController::IsDualCastPerk(RE::BGSPerk *perk, RE::ActorValue school) {
        // The perk must have the dual cast entry point
        bool hasDualCastEntry = false;
        auto size = perk->perkEntries.size();
        for (decltype(size) i = 0; i < size; i++) {
            auto entry = perk->perkEntries[i];
            if (entry->GetType() != RE::PERK_ENTRY_TYPE::kEntryPoint) {
                continue;
            }
            auto entryPointEntry = static_cast<RE::BGSEntryPointPerkEntry *>(entry);

            if (entryPointEntry->entryData.entryPoint == RE::BGSEntryPoint::ENTRY_POINTS::ENTRY_POINT::kCanDualCastSpell) {
                hasDualCastEntry = true;
                break;
            }
        }

        if (!hasDualCastEntry) {
            return false;
        }

        // And additionally it must have a condition checking the player's skill level
        // in the skill representing the spell school
        auto condition = perk->perkConditions.head;
        while (condition) {
            auto data = condition->data;
            if (
                data.functionData.function.any<RE::FUNCTION_DATA::FunctionID>(RE::FUNCTION_DATA::FunctionID::kGetBaseAV) ||
                data.functionData.function.any(RE::FUNCTION_DATA::FunctionID::kGetBaseActorValue)
            ) {
                // For the GetAV related functions the first argument is the actor value to check
                auto conditionAV = static_cast<RE::ActorValue>(
                    reinterpret_cast<std::uintptr_t>(data.functionData.params[0])
                );
                if (conditionAV == school) {
                    return true;
                }
            }

            condition = condition->next;
        }

        return false;
    }

    bool SpellCastController::CanDualCast(RE::PlayerCharacter* player, RE::SpellItem* spell) {
        if (!spell || !player) {
            return false;
        }

        // Scrolls can always be dual cast
        if (spell->GetFormType() == RE::FormType::Scroll) {
            return true;
        }

        // The spell must support dual casting
        if (spell->GetNoDualCastModifications()) {
            return false;
        }

        // The player must have the relevant dual casting perk
        bool hasDualCastPerk = false;
        auto perks = player->GetPlayerRuntimeData().addedPerks;
        auto size = perks.size();
        for (decltype(size) i = 0; i < size; i++) {
            auto perk = perks[i];
            if (perk->currentRank < 1) {
                continue;
            }
            if (IsDualCastPerk(perk->perk, spell->GetAssociatedSkill())) {
                hasDualCastPerk = true;
                break;
            }
        }

        if (!hasDualCastPerk) {
            return false;
        }

        RE::MagicSystem::CannotCastReason reason;
        bool ok = player->CheckCast(spell, true, &reason);
        return ok && reason == RE::MagicSystem::CannotCastReason::kOK;
    }

    

    bool SpellCastController::HasSpell(RE::PlayerCharacter* player, const SkillCastRequest &request) {
        if (!player || request.isPower || !request.spell) {
            return false;
        }

        // For scrolls, test if the player has the scroll in their inventory
        if (request.isScroll) {
            auto inventory = player->GetInventory();
            for (auto &[object, data] : inventory) {
                if (object->GetFormType() != RE::FormType::Scroll) {
                    continue;
                }

                // If there are no remaining scrolls of this kind, don't allow equipping
                if (data.first == 0) {
                    continue;
                }

                if (object->GetFormID() == request.spell->GetFormID()) {
                    return true;
                }
            }

            ScrollLog("Player doesn't own scroll %d", request.spell->GetFormID());

            return false;
        }

        // If the player knows the spell, return true directly
        if (player->HasSpell(request.spell)) {
            return true;
        }
        // Otherwise only return true if the player has the spell in the request's hand
        else {
            switch (request.castingSource) {
                case SkillCastSource::LeftHand: {
                    auto leftSpell = GetEquippedSpell(player, RE::MagicSystem::CastingSource::kLeftHand);
                    return leftSpell == request.spell;
                }
                case SkillCastSource::RightHand: {
                    auto rightSpell = GetEquippedSpell(player, RE::MagicSystem::CastingSource::kRightHand);
                    return rightSpell == request.spell;
                }
                case SkillCastSource::BothHands: {
                    auto rightSpell = GetEquippedSpell(player, RE::MagicSystem::CastingSource::kRightHand);
                    auto leftSpell = GetEquippedSpell(player, RE::MagicSystem::CastingSource::kLeftHand);
                    return rightSpell == request.spell && rightSpell == leftSpell;
                }
                default:
                    return false;
            }
        }
    }

    float SpellCastController::GetChargeTime(RE::SpellItem *spell) {
        if (!spell) {
            return 0.0f;
        }

        return spell->GetChargeTime();
    }

    bool SpellCastController::IsPowerSpell(RE::SpellItem *spell) {
        auto type = spell->GetSpellType();
        return (
            type == RE::MagicSystem::SpellType::kAbility ||
            type == RE::MagicSystem::SpellType::kLesserPower ||
            type == RE::MagicSystem::SpellType::kPower
        );
    }

    SkillCastSource SpellCastController::GetRequestActualCastingSource(SkillCastRequest request) {
        auto actualCastingSource = request.castingSource;
        if (
            request.castingSource == SkillCastSource::BothHands &&
            !request.isPower &&
            request.spell &&
            !request.spell->IsTwoHanded()
        ) {
            auto player = RE::PlayerCharacter::GetSingleton();
            if (player && !CanDualCast(player, request.spell)) {
                actualCastingSource = SkillCastSource::RightHand;
            }
        }

        return actualCastingSource;
    }

    // MARK: Hooks

    void SpellCastController::InstallHooks() {
        // Install the main loop hook
        #ifdef SpellCastMainLoopHook
            auto &trampoline = SKSE::GetTrampoline();
            const REL::Relocation<uintptr_t> mainHook { REL::VariantID(35565, 36564, 0x5BAB10) };
            _baseMainLoop = trampoline.write_call<5>(mainHook.address() + REL::VariantOffset(0x748, 0xC26, 0x7EE).offset(), MainLoop);
        #endif

        // Install the process button hook
        REL::Relocation<uintptr_t> attackBlockVtable { RE::VTABLE_AttackBlockHandler[0] };
        _baseProcessButton = attackBlockVtable.write_vfunc(0x4, ProcessButton);

        // Install the animation event hook
        REL::Relocation<uintptr_t> animationEventVtable { RE::VTABLE_PlayerCharacter[2] };
        _baseProcessAnimationEvent = animationEventVtable.write_vfunc(0x1, ProcessAnimationEvent);

        #ifdef AnimationSkips
            REL::Relocation<std::uintptr_t> PlayerCharacterVtbl { RE::VTABLE_PlayerCharacter[0] };
            _baseUpdateAnimation = PlayerCharacterVtbl.write_vfunc(0x7D, UpdateAnimation);
        #endif

    }

    #ifdef SpellCastMainLoopHook
        void SpellCastController::MainLoop() {
            _baseMainLoop();
            SharedController()->ProcessFrame();
        }
    #endif

    RE::BSEventNotifyControl SpellCastController::ProcessAnimationEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent> *sink, RE::BSAnimationGraphEvent *event, RE::BSTEventSource<RE::BSAnimationGraphEvent> *source) {
        auto result = SpellCastController::SharedController()->ProcessEvent(event, source);
        _baseProcessAnimationEvent(sink, event, source);

        return result;
    }

    
    #ifdef AnimationSkips
        void SpellCastController::UpdateAnimation(RE::Actor* thisActor, float delta) {
            if (skipsAnimationStep && false) {
                skipsAnimationStep--;
                // This hacky line just causes the current animation (previously set by the equipment manager)
                // to instantly skip 1000s, effectively bypassing the spell equipment animation
                _baseUpdateAnimation(thisActor, delta + 1000.0f);
            }
            else {
                _baseUpdateAnimation(thisActor, delta);
            }
        }
    #endif

    void SpellCastController::ProcessButton(RE::AttackBlockHandler *obj, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data) {        
        auto controller = SpellCastController::SharedController();

        // Awalys pass the event through while the player is transformed or
        // controls are disabled
        if (InputEventController::IsTransformed() || !InputEventController::IsPlayerInteractionEnabled()) {
            return _baseProcessButton(obj, a_event, a_data);
        }
        
        // For non-controller events, disable processing if queuing is enabled
        // for the event's hand
        if (!controller->_isControllerEvent) {
            if (
                !controller->_configuration.autoCastsEquippedSpell ||
                controller->_configuration.castBehaviour == SkillCastBehaviour::Equip
            ) {
                return _baseProcessButton(obj, a_event, a_data);
            }

            unsigned hand = InputEventController::SharedController()->GetHandForButtonEvent(a_event);
            if (hand == 0) {
                return _baseProcessButton(obj, a_event, a_data);
            }


            RE::SpellItem *leftSpell = nullptr;
            RE::SpellItem *rightSpell = nullptr;

            {     
                std::lock_guard lock(controller->_storedEquipmentLock);
                // If there is a stored or equipped spell for the specified hand, ignore this request 
                if (controller->_storedEquipment) {
                    auto leftItem = controller->_storedEquipment->SpellItemInHand(EquippedItemHand::LeftHand);
                    if (leftItem && leftItem->GetFormType() == RE::FormType::Spell) {
                        leftSpell = leftItem->As<RE::SpellItem>();
                    }
                    else {
                        leftSpell = nullptr;
                    }

                    auto rightItem = controller->_storedEquipment->SpellItemInHand(EquippedItemHand::RightHand);
                    if (rightItem && rightItem->GetFormType() == RE::FormType::Spell) {
                        rightSpell = rightItem->As<RE::SpellItem>();
                    }
                    else {
                        rightSpell = nullptr;
                    }
                }
                else {
                    // Otherwise use the player's current equipment
                    auto player = RE::PlayerCharacter::GetSingleton();
                    if (!player) {
                        return _baseProcessButton(obj, a_event, a_data);
                    }

                    leftSpell = controller->GetEquippedSpell(player, RE::MagicSystem::CastingSource::kLeftHand);
                    rightSpell = controller->GetEquippedSpell(player, RE::MagicSystem::CastingSource::kRightHand);
                }
            }

            // Disable processing if a spell is in the requested hand
            switch ((SkillCastSource)hand) {
                case SkillCastSource::LeftHand: {
                    if (leftSpell) {
                        return;
                    }
                    break;
                }
                case SkillCastSource::RightHand: {
                    if (rightSpell) {
                        return;
                    }
                    break;
                }
                case SkillCastSource::BothHands: {
                    if (leftSpell && rightSpell) {
                        return;
                    }
                    break;
                }
                default: {
                    break;
                }
            }
        }

        return _baseProcessButton(obj, a_event, a_data);
    }

    // MARK: Input handlers

    void SpellCastController::QueueSkill(RE::FormID skillID, RE::MagicSystem::CastingSource hand, uint32_t key) {
        if (skillID == 0) {
            return;
        }

        auto form = RE::TESForm::LookupByID(skillID);
        if (!form) {
            return;
        }

        switch (form->GetFormType()) {
            case RE::FormType::Spell: {
                // Create and queue a spell cast request for this spell
                auto spell = form->As<RE::SpellItem>();
                if (!spell) {
                    return;
                }

                SkillCastRequest request;
                request.initialized = true;
                request.spell = spell;
                request.isPower = IsPowerSpell(spell);
                if (request.isPower) {
                    request.castingSource = SkillCastSource::Power;
                    request.manuallyReleased = true;
                    request.aftercastDelayFinished = AnimationWaitingState::Ready;
                }
                else {
                    if (spell->IsTwoHanded()){
                        // A two handed spell is required to be dual cast
                        request.castingSource = SkillCastSource::BothHands;
                    }
                    else {
                        request.castingSource = SkillCastSourceWithMagicSource(hand);
                    }
                    // Concentration spells must be held and manually released, all others
                    // are handled by the automated casting if enabled
                    request.manuallyReleased = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;

                    // Apply any overrides to the spell if defined
                    if (_spellOverrideMap.contains(form->GetFormID())) {
                        auto overrides = _spellOverrideMap.at(form->GetFormID());

                        if (overrides.overridesManuallyReleased) {
                            request.manuallyReleased = overrides.manuallyReleased;
                        }
                    }

                    // If the spell is manually released, it has no aftercast delay
                    if (request.manuallyReleased) {
                        request.aftercastDelayFinished = AnimationWaitingState::Ready;
                    }
                }
                request.key = key;
                request.keyHeld = true;
                QueueRequest(
                    request,
                    request.isPower || _configuration.castBehaviour != SkillCastBehaviour::CastAndReequip
                );
                break;
            }
            case RE::FormType::Shout: {
                // Create and queue a spell cast request for this shout
                auto shout = form->As<RE::TESShout>();
                if (!shout) {
                    return;
                }

                SkillCastRequest request;
                request.castingSource = SkillCastSource::Shout;
                request.initialized = true;
                request.isPower = true;
                request.manuallyReleased = true;
                request.shout = shout;
                request.key = key;
                request.keyHeld = true;
                request.aftercastDelayFinished = AnimationWaitingState::Ready;
                QueueRequest(request);
                break;
            }
        }
    }

    void SpellCastController::CastScroll(RE::FormID formID, RE::MagicSystem::CastingSource hand) {
        // Create and queue a spell cast request for this scroll
        auto form = RE::TESForm::LookupByID(formID);
        if (!form) {
            return;
        }

        auto spell = form->As<RE::ScrollItem>();
        if (!spell) {
            return;
        }

        SkillCastRequest request;
        request.initialized = true;
        request.spell = spell;
        request.isPower = false;
        request.isScroll = true;

        if (spell->IsTwoHanded()){
            // A two handed spell is required to be dual cast
            request.castingSource = SkillCastSource::BothHands;
        }
        else {
            request.castingSource = SkillCastSourceWithMagicSource(hand);
        }

        request.manuallyReleased = false;

        QueueRequest(
            request,
            _configuration.castBehaviour != SkillCastBehaviour::CastAndReequip
        );
    }

    void SpellCastController::KeyPressed(uint32_t key) {
        std::lock_guard lock(_keyLock);

        // Clear the key flag on the appropriate requests if this key matches theirs
        if (_currentSpell.key == key) {
            _currentSpell.keyHeld = true;
        }
        // Clear the key flag on the appropriate requests if this key matches theirs
        if (_queuedSpell.key == key) {
            _queuedSpell.keyHeld = true;
        }
    }

    void SpellCastController::KeyReleased(uint32_t key) {
        std::lock_guard lock(_keyLock);

        // Clear the key flag on the appropriate requests if this key matches theirs
        if (_currentSpell.key == key) {
            _currentSpell.keyHeld = false;
        }
        // Clear the key flag on the appropriate requests if this key matches theirs
        if (_queuedSpell.key == key) {
            _queuedSpell.keyHeld = false;
        }
    }

    void SpellCastController::UseConsumable(RE::FormID formID, RE::MagicSystem::CastingSource hand, PotionSlotConfiguration potion) {
        if (potion.kind == PotionSlotItemKind::Scroll) {
            if (formID != 0) {
                CastScroll(formID, hand);
            }
            return;
        }

        return ConsumePotion(formID, potion);
    }

    void SpellCastController::ConsumePotion(RE::FormID formID, PotionSlotConfiguration potion) {
        if (formID == 0 && potion.kind == PotionSlotItemKind::None && potion.effect == RE::ActorValue::kNone) {
            return;
        }

        float highestMagnitude = 0.0f;
        RE::AlchemyItem *potionToUse = nullptr;
        RE::ExtraDataList *extraDataToUse = nullptr;

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return;
        }

        auto inventory = player->GetInventory();
        for (auto &[object, data] : inventory) {
            if (object->GetFormType() != RE::FormType::AlchemyItem) {
                continue;
            }

            // If there are no remaining potions of this kind, don't consume
            if (data.first == 0) {
                continue;
            }

            auto item = object->As<RE::AlchemyItem>();
            float magnitude = InputEventController::PotionMagnitudeForConfiguration(item, potion);
            bool isTargetPotion = false;

            if (formID != 0) {
                if (object->formID == formID) {
                    if (magnitude > highestMagnitude) {
                        isTargetPotion = true;
                    }
                }
            }
            else {
                if (magnitude > highestMagnitude) {
                    isTargetPotion = true;
                }
            }

            if (!isTargetPotion) {
                continue;
            }
            
            highestMagnitude = magnitude;
            potionToUse = item;
            extraDataToUse = nullptr;
            auto extraData = data.second->extraLists;
            if (extraData) {
                for (auto &extraList : *extraData) {
                    extraDataToUse = extraList;
                }
            }
        }

        // If a matching potion was found, consume it
        if (potionToUse) {
            auto equipManager = RE::ActorEquipManager::GetSingleton();
            if (!equipManager) {
                player->DrinkPotion(potionToUse, extraDataToUse);
            }
            else {
                // "Equipping" the potion instead of consuming it is preferred because
                // otherwise the hunger value doesn't update correctly
                equipManager->EquipObject(player, potionToUse, extraDataToUse, 1, nullptr, false, false, true, false);
            }
        }
        else {
            PlayMagicFailSound(player, RE::MagicSystem::SpellType::kSpell);
        }
    }
    
    // MARK: Spell casting start

    RE::BGSEquipSlot *SpellCastController::GetLeftHandSlot() {
        using func_t = decltype(&GetLeftHandSlot);
        const REL::Relocation<func_t> func{ RELOCATION_ID(23150, 23607) };
        return func();
    }

    RE::BGSEquipSlot *SpellCastController::GetVoiceSlot() {
        using func_t = decltype(&GetVoiceSlot);
        const REL::Relocation<func_t> func{ RELOCATION_ID(23153, 23610) };
        return func();
    }

    RE::BGSEquipSlot *SpellCastController::GetRightHandSlot() {
        using func_t = decltype(&GetRightHandSlot);
        const REL::Relocation<func_t> func{ RELOCATION_ID(23151, 23608) };
        return func();
    }

    void SpellCastController::FlashMagickaBar() {
        // In addition to flashing the magicka bar, also play the magicka too low sound
        auto player = RE::PlayerCharacter::GetSingleton();
        if (player) {
            PlayMagicFailSound(player, RE::MagicSystem::SpellType::kSpell);
        }

        return FlashMeter(RE::ActorValue::kMagicka);
    }

    void SpellCastController::PlayMagicFailSound(RE::PlayerCharacter *player, RE::MagicSystem::SpellType spellType) {
        // This function is missing from this version of Commonlib, but newer versions
        // completely remove the render manager header
        using func_t = decltype(&SpellCastController::PlayMagicFailSound);
        REL::Relocation<func_t> func { RELOCATION_ID(39486, 40565) };
        return func(player, spellType);
    }

    void SpellCastController::FlashMeter(RE::ActorValue actorValue) {
        // This function is missing from this version of Commonlib, but newer versions
        // completely remove the render manager header
        using func_t = decltype(&SpellCastController::FlashMeter);
        REL::Relocation<func_t> func { RELOCATION_ID(51907, 52845) };
        return func(actorValue);
    }

    void SpellCastController::QueueRequest(SkillCastRequest request, bool skipsEquipmentSave) {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return;
        }

        auto state = player->AsActorState();
        if (!state) {
            return;
        }

        // Don't attempt skills mid-jump, on horse, swimming, disabled or during kill cam
        // TODO: or while dodging
        if (
            player->IsInMidair() ||
            state->IsSwimming() ||
            state->GetKnockState() != RE::KNOCK_STATE_ENUM::kNormal ||
            state->actorState1.sitSleepState == RE::SIT_SLEEP_STATE::kRidingMount ||
            player->GetActorRuntimeData().boolFlags.all(RE::Actor::BOOL_FLAGS::kIsInKillMove)
        ) {
            return;
        }

        // If the behaviour is to just equip, equip now and return
        if (_configuration.castBehaviour == SkillCastBehaviour::Equip) {
            auto equipManager = RE::ActorEquipManager::GetSingleton();
            if (!equipManager) {
                return;
            }

            if (request.isPower) {
                if (request.spell) {
                    _isControllerEquipEvent = true;
                    equipManager->EquipSpell(player, request.spell, GetRightHandSlot());
                }
                else if (request.shout) {
                    _isControllerEquipEvent = true;
                    equipManager->EquipShout(player, request.shout);
                }
            }
            else {
                if (request.isScroll) {
                    _isControllerEquipEvent = true;
                    if (HasSpell(player, request)) {
                        equipManager->EquipObject(player, request.spell, nullptr, 1, GetRightHandSlot(), false, true, false);
                    }
                }
                else {
                    switch (request.castingSource) {
                        case SkillCastSource::LeftHand:
                            _isControllerEquipEvent = true;
                            equipManager->EquipSpell(player, request.spell, GetLeftHandSlot());
                            break;
                        case SkillCastSource::RightHand:
                            _isControllerEquipEvent = true;
                            equipManager->EquipSpell(player, request.spell, GetRightHandSlot());
                            break;
                        case SkillCastSource::BothHands:
                            _isControllerEquipEvent = true;
                            equipManager->EquipSpell(player, request.spell, GetRightHandSlot());
                            equipManager->EquipSpell(player, request.spell, GetLeftHandSlot());
                            break;
                        default:
                            return;
                    }
                }
            }
            return;
        }

        // If the request is an instant power and the current in progress cast is not a power
        // Immediately apply and cast it
        if (request.isPower && request.spell && request.spell->GetChargeTime() == 0.0f) {
            if (!_currentSpell.initialized || !_currentSpell.isPower) {
                CastInstantPower(request);
                return;
            }
        }

        // If a spell is already in progress or the controller is waiting for disable
        // bumper, queue this cast
        if (_currentSpell.initialized && !_currentSpell.finalized) {
            RuntimeCastLog("Queueing request because the current spell is not finalized");
            _queuedSpell = request;
            _queuedSpell.initialized = true;
            _queuedSpell.casting = false;
            _queuedSpell.castingStarted = false;
            return;
        }

        // If this is a left handed spell and the controller is waiting for the enable
        // bumper event, queue this cast
        _drawWeaponLock.lock();
        if (
            (request.castingSource == SkillCastSource::LeftHand) &&
            (_waitingDrawWeapons != AnimationWaitingState::None)
        ) {
            RuntimeCastLog("Queueing request because controller is waiting for bumper enable");
            _queuedSpell = request;
            _queuedSpell.initialized = true;
            _queuedSpell.casting = false;
            _queuedSpell.castingStarted = false;
            _drawWeaponLock.unlock();
            return;
        }
        _drawWeaponLock.unlock();

        // For non two-handed dual cast requests, ensure that a dual cast is possible
        // to check for aftercast, otherwise the queued request registers as a different
        // source from the current spell, but it would be coerced to the correct one
        // when the request actually gets fulfilled
        auto actualCastingSource = GetRequestActualCastingSource(request);
        
        if (
            !_currentSpell.isPower &&
            !request.isPower &&
            _currentSpell.spell == request.spell &&
            _currentSpell.castingSource == actualCastingSource &&
            _currentSpell.finalized
        ) {
            RuntimeCastLog("Queueing request because aftercast delay hasn't finished");
            _queuedSpell = request;
            _queuedSpell.initialized = true;
            _queuedSpell.casting = false;
            _queuedSpell.castingStarted = false;
            return;
        }
        
        // Otherwise start casting it
        FulfillRequest(request, skipsEquipmentSave);
    }

    void SpellCastController::CastInstantPower(SkillCastRequest request) {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return;
        }

        auto equipManager = RE::ActorEquipManager::GetSingleton();
        if (!equipManager) {
            return;
        }
        
        // Test if the spell can be casted at all, otherwise cancel this request
        RE::MagicSystem::CannotCastReason reason;
        bool ok = player->CheckCast(request.spell, false, &reason);
        if (!ok || reason != RE::MagicSystem::CannotCastReason::kOK) {
            return;
        }

        auto currentPower = player->GetActorRuntimeData().selectedPower;
        _isControllerEquipEvent = true;
        equipManager->EquipSpell(player, request.spell, GetRightHandSlot());

        // Send a very fast power button tap
        SendAttackBlockButton(SkillCastSource::Power, 1.0f, 0.0f);
        SKSE::GetTaskInterface()->AddTask([currentPower, this]() {
            SharedController()->SendAttackBlockButton(SkillCastSource::Power, 1.0f, 0.016f);
            SharedController()->SendAttackBlockButton(SkillCastSource::Power, 0.0f, 0.016f);

            if (!currentPower) {
                return;
            }

            // Always attempt to assign back the previous power in this case
            auto player = RE::PlayerCharacter::GetSingleton();
            if (!player) {
                return;
            }

            auto equipManager = RE::ActorEquipManager::GetSingleton();
            if (!equipManager) {
                return;
            }

            _isControllerEquipEvent = true;
            if (currentPower->GetFormType() == RE::FormType::Shout) {
                equipManager->EquipShout(player, currentPower->As<RE::TESShout>());
            }
            else {
                equipManager->EquipSpell(player, currentPower->As<RE::SpellItem>(), GetRightHandSlot());
            }
        });
    }

    void SpellCastController::FulfillRequest(SkillCastRequest request, bool skipsEquipmentSave) {
        if (request.isPower) {
            FulfillPower(request, skipsEquipmentSave);
        }
        else {
            FulfillSpell(request, skipsEquipmentSave);
        }
    }

    void SpellCastController::FulfillPower(SkillCastRequest request, bool skipsEquipmentSave) {
        auto player = RE::PlayerCharacter::GetSingleton();
        auto equipManager = RE::ActorEquipManager::GetSingleton();
        if (!player || !equipManager) {
            return;
        }

        // Equip the spell/power if needed
        auto currentPower = player->GetActorRuntimeData().selectedPower;
        auto formID = request.spell ? request.spell->formID : request.shout->formID;
        bool powerChanged = false;

        if (!currentPower || currentPower->formID != formID) {
            powerChanged = true;
            if (request.spell) {
                // If the player doesn't own the spell, don't continue
                if (!player->HasSpell(request.spell)) {
                    return;
                }

                // Test if the spell can be casted at all, otherwise cancel this request
                RE::MagicSystem::CannotCastReason reason;
                bool ok = player->CheckCast(request.spell, false, &reason);
                if (!ok || reason != RE::MagicSystem::CannotCastReason::kOK) {

                    // If the reason is insufficient magicka, flash the magicka bar
                    if (reason == RE::MagicSystem::CannotCastReason::kMagicka) {
                        FlashMagickaBar();
                    }

                    return;
                }

                _isControllerEquipEvent = true;
                equipManager->EquipSpell(player, request.spell, GetRightHandSlot());
            }
            else {
                _isControllerEquipEvent = true;
                equipManager->EquipShout(player, request.shout);
            }
        }

        if (!skipsEquipmentSave && powerChanged) {
            // TODO: Store and restore power
        }
        
        _currentSpell = request;
        _currentSpell.casting = true;
        _currentSpell.castingStarted = false;
        _currentSpell.keypressStart = *engineTime;
        
        // Shouts and powers don't need weapons drawn, so casting can be initiated instantly
        SendAttackBlockButton(request.castingSource, 1.0f, 0.0f);
    }

    void SpellCastController::FulfillSpell(SkillCastRequest request, bool skipsEquipmentSave) {
        auto player = RE::PlayerCharacter::GetSingleton();
        auto equipManager = RE::ActorEquipManager::GetSingleton();
        auto state = player->AsActorState();
        if (!player || !equipManager || !state) {
            return;
        }

        // If the player doesn't own the spell, don't continue
        if (!HasSpell(player, request)) {

            // If the request is a scroll, also play the spell fail sound so players
            // know they don't have the scroll anymore
            if (request.isScroll) {
                PlayMagicFailSound(player, RE::MagicSystem::SpellType::kSpell);
            }

            return;
        }

        // Equip the spell if needed
        bool equipsLeftHand = request.castingSource == SkillCastSource::LeftHand || request.castingSource == SkillCastSource::BothHands;
        bool equipsRightHand = request.castingSource == SkillCastSource::RightHand || request.castingSource == SkillCastSource::BothHands;
        bool canDualCast = false;

        // If the spell is dual cast but the player is not able to dual cast it
        // change the equipment to right hand only
        if (request.castingSource == SkillCastSource::BothHands) {
            canDualCast = CanDualCast(player, request.spell);
            if (!canDualCast && !request.spell->IsTwoHanded()) {
                request.castingSource = SkillCastSource::RightHand;
                equipsLeftHand = false;
            }
        }

        if (!request.isScroll) {
            // Test if the spell can be casted at all, otherwise cancel this request
            RE::MagicSystem::CannotCastReason reason;
            bool ok = player->CheckCast(request.spell, canDualCast && request.castingSource == SkillCastSource::BothHands, &reason);
            if (!ok || reason != RE::MagicSystem::CannotCastReason::kOK) {
                
                // If the reason is insufficient magicka, flash the magicka bar
                if (reason == RE::MagicSystem::CannotCastReason::kMagicka) {
                    FlashMagickaBar();
                }

                return;
            }
        }

        auto weaponsDrawn = state->IsWeaponDrawn();
        auto weaponsDrawing = 
                state->GetWeaponState() == RE::WEAPON_STATE::kSheathing ||
                state->GetWeaponState() == RE::WEAPON_STATE::kDrawing;

        bool leftHandChanged = false;
        bool rightHandChanged = false;

        bool hasMainHandWeapon = false;
        bool hasStaff = false;

        {
            std::lock_guard lock(_storedEquipmentLock);

            // Take a snapshot of the current equipment if reequipping is enabled
            auto currentEquipment = new StoredEquipment();

            hasMainHandWeapon = currentEquipment->ContainsWeaponMainHand();
            hasStaff = currentEquipment->ContainsStaffItem();

            // If the current equipment is two handed and this is a left hand request, equip it
            // in both hands, this avoids a potential issue where a one handed melee weapon is the backup
            // main hand weapon and it plays its long drawing animation
            if (request.castingSource == SkillCastSource::LeftHand && currentEquipment->ContainsTwoHandedItem()) {
                equipsRightHand = true;
            }

            // If reequiping is not enabled, discard the stored equipment
            if (_configuration.castBehaviour != SkillCastBehaviour::CastAndReequip) {
                delete currentEquipment;
                currentEquipment = nullptr;
            }
            
            auto leftEquippedSpell = GetEquippedSpell(player, RE::MagicSystem::CastingSource::kLeftHand);
            auto rightEquippedSpell = GetEquippedSpell(player, RE::MagicSystem::CastingSource::kRightHand);

            _drawWeaponLock.lock();
            if (equipsLeftHand) {
                if (!leftEquippedSpell || leftEquippedSpell->formID != request.spell->formID) {
                    leftHandChanged = true;

                    // If the controller is waiting to equip weapons, queue this request instead
                    if (!equipsRightHand && _waitingDrawWeapons == AnimationWaitingState::Waiting) {
                        _queuedSpell = request;
                        _drawWeaponLock.unlock();
                        return;
                    }

                    // The only time scrolls can be equipped in the left hand is if they are two
                    // handed, and that is handled already by the right hand equip
                    if (!request.isScroll) {
                        _isControllerEquipEvent = true;
                        equipManager->EquipSpell(player, request.spell, GetLeftHandSlot());
                    }
                }
            }
            if (equipsRightHand) {
                if (!rightEquippedSpell || rightEquippedSpell->formID != request.spell->formID) {
                    rightHandChanged = true;
                    _isControllerEquipEvent = true;
                    if (request.isScroll) {
                        equipManager->EquipObject(player, request.spell, nullptr, 1, GetRightHandSlot(), false, true, false);
                    }
                    else {
                        equipManager->EquipSpell(player, request.spell, GetRightHandSlot());
                    }
                    _waitingDrawWeapons = AnimationWaitingState::None;
                }
            }
            _drawWeaponLock.unlock();

            #ifdef AnimationSkips
                // Only allow skipping the animation when it's between spells
                bool canSkipAnimation = true;
                if (leftHandChanged && !leftEquippedSpell) {
                    canSkipAnimation = false;
                }
                if (rightHandChanged && !rightEquippedSpell) {
                    canSkipAnimation = false;
                }

                if ((leftHandChanged || rightHandChanged) && canSkipAnimation) {
                    skipsAnimationStep = 2;
                }
            #endif

            if (!skipsEquipmentSave && (leftHandChanged || rightHandChanged)) {
                if (!_storedEquipment) {
                    AutocastLog("Stored new equipment");
                    _storedEquipment = currentEquipment;
                }
                else {
                    if (currentEquipment) {
                        delete currentEquipment;
                    }
                    RuntimeCastLog("Unexpected request to store equipment while there is already stored equipment.");
                }
            }
            else if (currentEquipment) {
                delete currentEquipment;
            }
        }

        // Play the sound effect of the previously fulfilled spell, if any
        PlaySpellFireSoundIfNeeded(leftHandChanged, rightHandChanged);

        _currentSpell = request;
        _queuedSpell = {};
        _currentSpell.chargeTime = GetChargeTime(request.spell);
        _currentSpell.casting = true;
        _currentSpell.castingStarted = false;
        _currentSpell.keypressStart = *engineTime;
        
        if (leftHandChanged || rightHandChanged) {
            // If the original equipment included a staff, add some additional
            // warm up delay, which appears to be required with some modded setups
            // and there doesn't seem to be a better method to determine when it would
            // be safe to actually start casting
            if (hasStaff) {
                _currentSpell.startTime = *engineTime + StaffStartWindupTimeout;
            }

            // If any weapons changed, must wait for either the drawing animation or a
            // "bumper disabled" animation event, depending on the initial state of the
            // weapons
            if (weaponsDrawing) {
                RuntimeCastLog("Waiting for weapon drawn");
                _currentSpell.waitingReadyWeapons = true;
            }
            else if (!weaponsDrawn) {
                // If the weapon is not drawn must wait for both bumper then weapons ready
                player->DrawWeaponMagicHands(true);
                RuntimeCastLog("Waiting for weapon drawn");
                _currentSpell.waitingDisableBumper = AnimationWaitingState::Waiting;
                _currentSpell.waitingReadyWeapons = true;
            }
            else {
                RuntimeCastLog("Waiting for disable bumper");
                _currentSpell.waitingDisableBumper = AnimationWaitingState::Waiting;

                // When dual casting from a weapon item, it is also required to wait for ready weapons
                if (hasMainHandWeapon && _currentSpell.castingSource == SkillCastSource::BothHands) {
                    player->DrawWeaponMagicHands(true);
                    _currentSpell.waitingReadyWeapons = true;
                }
            }
        }
        else if (!weaponsDrawn) {
            RuntimeCastLog("Waiting for weapon drawn");
            // If the weapon is not drawn or any equipment was changed, draw it now
            player->DrawWeaponMagicHands(true);
            _currentSpell.waitingReadyWeapons = true;
        }
        else {
            // Otherwise initiate the cast
            SendAttackBlockButton(request.castingSource, 1.0f, 0.0f);
            _currentSpell.keypressStarted = true;
        }

    }

    bool SpellCastController::CastQueuedSpell() {
        if (!_queuedSpell.initialized) {
            return false;
        }

        // If the queued spell is a concentration but its key is not held, dismiss it
        if (_queuedSpell.manuallyReleased && !_queuedSpell.keyHeld) {
            _queuedSpell = {};
            return false;
        }

        FulfillRequest(_queuedSpell, !!_storedEquipment);
        _queuedSpell = {};
        return true;
    }

    // MARK: Virtual events

    const RE::BSFixedString &SpellCastController::AttackUserEvent(SkillCastSource source) {
        auto *ue = RE::UserEvents::GetSingleton();
        switch (source) {
            case SkillCastSource::LeftHand:
                return ue->leftAttack;
            case SkillCastSource::RightHand:
                return ue->rightAttack;
            case SkillCastSource::Power:
                return ue->forceRelease;
            case SkillCastSource::Shout:
                return ue->shout;
            default:
                return ue->dualAttack;
        }
    }

    inline void SpellCastController::SendAttackBlockButton(SkillCastSource source, float state, float heldDownSecs) {
        // If the request source is dual, separately send both the left and right sources
        if (source == SkillCastSource::BothHands) {
            SendAttackBlockButton(SkillCastSource::RightHand, state, heldDownSecs);
            SendAttackBlockButton(SkillCastSource::LeftHand, state, heldDownSecs);
            return;
        }
        
        auto *player = RE::PlayerCharacter::GetSingleton();
        auto *controls = RE::PlayerControls::GetSingleton();

        if (!player || !controls || !controls->attackBlockHandler) {
            return;
        }

        // Don't allow sending events while the UI is open
        if (auto *ui = RE::UI::GetSingleton(); ui && ui->GameIsPaused()) {
            return;
        }

        const auto &userEvent = AttackUserEvent(source);

        auto keybind = InputEventController::SharedController()->GetKeybindForEvent(userEvent);

        if (state == 0.0f || heldDownSecs == 0.0f) {
            RuntimeCastLog("Sending keypress of type %s state %f, held %f", userEvent.c_str(), state, heldDownSecs);
        }

        auto *event = RE::ButtonEvent::Create(keybind.device, userEvent, keybind.keyID, state, heldDownSecs);
        if (!event) {
            return;
        }

        switch (source) {
            case SkillCastSource::Power:
                [[fallthrough]];
            case SkillCastSource::Shout: {
                auto *shoutHandler = controls->shoutHandler;
                shoutHandler->ProcessButton(event, &controls->data);
                break;
            }
            default: {
                _isControllerEvent = true;
                auto *attackHandler = controls->attackBlockHandler;
                attackHandler->ProcessButton(event, &controls->data);
                _isControllerEvent = false;
            }
        }

        RE::free(event);
    }

    inline bool SpellCastController::RestoreEquipment() {
        std::lock_guard lock(_storedEquipmentLock);

        if (!_storedEquipment || _configuration.castBehaviour != SkillCastBehaviour::CastAndReequip) {
            return false;
        }

        auto leftSpell = _storedEquipment->SpellItemInHand(EquippedItemHand::LeftHand);
        auto rightSpell = _storedEquipment->SpellItemInHand(EquippedItemHand::RightHand);

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return false;
        }
        auto currentLeftSpell = GetEquippedSpell(player, RE::MagicSystem::CastingSource::kLeftHand);
        auto currentRightSpell = GetEquippedSpell(player, RE::MagicSystem::CastingSource::kRightHand);

        #ifdef AnimationSkips
            // If both equipped items are spells, the animation can be skipped
            if (
                _storedEquipment->SpellItemInHand(EquippedItemHand::LeftHand) &&
                _storedEquipment->SpellItemInHand(EquippedItemHand::RightHand)
            ) {
                skipsAnimationStep = 2;
            }
        #endif

        std::lock_guard lockWeapons(_drawWeaponLock);

        // Enable bumper only needs to wait if a martial weapon was equipped in the
        // main hand
        _isControllerEquipEvent = true;
        EquipmentLog("Restore equip");
        bool leftHandChanged = false;
        bool rightHandChanged = false;

        bool waitsDrawWeapons = _storedEquipment->Equip(&leftHandChanged, &rightHandChanged);

        EquipmentLog("Restore equip end");

        PlaySpellFireSoundIfNeeded(leftHandChanged, rightHandChanged);

        if (waitsDrawWeapons) {
            _waitingDrawWeapons = AnimationWaitingState::Waiting;
            _drawWeaponsStartTime = *engineTime;
        }

        return currentLeftSpell != leftSpell || currentRightSpell != rightSpell;
    }

    // MARK: Spell casting process

    void SpellCastController::FinishCasting() {
        if (_currentSpell.initialized) {
            _currentSpell.casting = false;
            _currentSpell.castingStarted = false;

            // Reuse the engine time to track and timeout the aftercast delay
            _currentSpell.holdStart = *engineTime;
            SendAttackBlockButton(_currentSpell.castingSource, 0.0f, *engineTime - _currentSpell.keypressStart);
        }
    }

    void SpellCastController::PlaySpellFireSoundIfNeeded(bool leftHandChanged, bool rightHandChanged) {
        // If more than the maximum interval elapsed since the spell was fulfilled, don't play any sound
        if (*engineTime - _fulfilledSpell.finishTime > SpellFireSoundTimeout) {
            SoundLog("Spell finish time %f exceeded the sound timeout", *engineTime - _fulfilledSpell.finishTime);
            return;
        }

        if (!_fulfilledSpell.spell || _fulfilledSpell.source == SkillCastSource::Power) {
            SoundLog("Fulfilled spell is not defined");
            return;
        }

        switch (_fulfilledSpell.source) {
            case SkillCastSource::BothHands:
                if (leftHandChanged || rightHandChanged) {
                    PlaySpellFireSound(_fulfilledSpell.spell);
                }
                break;
            case SkillCastSource::LeftHand:
                if (leftHandChanged) {
                    PlaySpellFireSound(_fulfilledSpell.spell);
                }
                break;
            case SkillCastSource::RightHand:
                if (rightHandChanged) {
                    PlaySpellFireSound(_fulfilledSpell.spell);
                }
                break;
        }
    }

    void SpellCastController::PlaySpellFireSound(RE::SpellItem *spell) {
        if (!spell || !spell->avEffectSetting) {
            SoundLog("Spell has no av effect");
            return;
        }

        auto audioManager = RE::BSAudioManager::GetSingleton();
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!audioManager || !player) {
            SoundLog("Could not obtain player or audio manager");
            return;
        }

        for (auto &sound : spell->avEffectSetting->effectSounds) {
            if (sound.id == RE::MagicSystem::SoundID::kRelease) {
                auto descriptor = sound.sound;
                if (!descriptor) {
                    SoundLog("Spell has no release descriptor");
                    return;
                }

                RE::BSSoundHandle handle;
                if (audioManager->BuildSoundDataFromDescriptor(handle, descriptor)) {
                    handle.SetObjectToFollow(player->Get3D2());
                    handle.Play();
                }
                else {
                    audioManager->Play(descriptor);
                }
            }
        }
    }

    void SpellCastController::CancelOrQueueCasting(unsigned hand, uint32_t key, bool restoreEquipment) {
        // If this setting is disabled, no hand is specified or automatic casting is
        // disabled for the skillbar, just use this to cancel casting
        if (
            !_configuration.autoCastsEquippedSpell ||
            hand == 0 ||
            _configuration.castBehaviour == SkillCastBehaviour::Equip
        ) {
            return CancelCasting(restoreEquipment);
        }

        // If equipment is currently stored, determine if the stored equipment has
        // a spell in the specified hand
        auto prefersDual = _configuration.prefersDualCast;
        RE::SpellItem *leftSpell = nullptr;
        RE::SpellItem *rightSpell = nullptr;

        {
            std::lock_guard lock(_storedEquipmentLock);
            if (_storedEquipment) {
                auto leftItem = _storedEquipment->SpellItemInHand(EquippedItemHand::LeftHand);
                if (leftItem && leftItem->GetFormType() == RE::FormType::Spell) {
                    leftSpell = leftItem->As<RE::SpellItem>();
                }
                else {
                    leftSpell = nullptr;
                }

                auto rightItem = _storedEquipment->SpellItemInHand(EquippedItemHand::RightHand);
                if (rightItem && rightItem->GetFormType() == RE::FormType::Spell) {
                    rightSpell = rightItem->As<RE::SpellItem>();
                }
                else {
                    rightSpell = nullptr;
                }
            }
            else {
                // Otherwise use the player's current equipment
                auto player = RE::PlayerCharacter::GetSingleton();
                if (player) {
                    leftSpell = GetEquippedSpell(player, RE::MagicSystem::CastingSource::kLeftHand);
                    rightSpell = GetEquippedSpell(player, RE::MagicSystem::CastingSource::kRightHand);
                }
            }
        }

        switch ((SkillCastSource)hand) {
            case SkillCastSource::LeftHand: {
                if (!leftSpell) {
                    return CancelCasting(true);
                }
                if (prefersDual) {
                    // If dual casting is requested, the spell must also be equipped in the other hand
                    if (leftSpell == rightSpell) {
                        return QueueSkill(leftSpell->formID, RE::MagicSystem::CastingSource::kOther, key);
                    }
                    else {
                        return QueueSkill(leftSpell->formID, RE::MagicSystem::CastingSource::kLeftHand, key);
                    }
                }
                else {
                    return QueueSkill(leftSpell->formID, RE::MagicSystem::CastingSource::kLeftHand, key);
                }
            }
            case SkillCastSource::RightHand: {
                if (!rightSpell) {
                    return CancelCasting(true);
                }
                if (prefersDual) {
                    // If dual casting is requested, the spell must also be equipped in the other hand
                    if (leftSpell == rightSpell) {
                        return QueueSkill(rightSpell->formID, RE::MagicSystem::CastingSource::kOther, key);
                    }
                    else {
                        return QueueSkill(rightSpell->formID, RE::MagicSystem::CastingSource::kRightHand, key);
                    }
                }
                else {
                    return QueueSkill(rightSpell->formID, RE::MagicSystem::CastingSource::kRightHand, key);
                }
            }
            case SkillCastSource::BothHands: {
                // This doesn't normally happen, in this case only queue if the same spell
                // is in both hands
                if (leftSpell && leftSpell == rightSpell) {
                    return QueueSkill(leftSpell->formID, RE::MagicSystem::CastingSource::kOther, key);
                }
                else {
                    return CancelCasting(true);
                }
            }
            default:
                return CancelCasting(true);
        }
    }

    void SpellCastController::CancelCasting(bool restoreEquipment) {
        // Clear any queued spell
        if (_queuedSpell.initialized) {
            _queuedSpell = {};
        }

        _drawWeaponLock.lock();
        _waitingDrawWeapons = AnimationWaitingState::None;
        _drawWeaponLock.unlock();

        // Cancel the currently casting spell, if any
        if (_currentSpell.initialized || _currentSpell.finalized) {
            if (_currentSpell.castingStarted) {
                SendAttackBlockButton(_currentSpell.castingSource, 0.0f, *engineTime - _currentSpell.keypressStart);
            }
            _currentSpell = {};
        }

        // Restore the saved equipment, if any
        if (restoreEquipment) {
            RestoreEquipment();
        }
    }

    void SpellCastController::ProcessFrame() {
        if (_isControllerEquipEvent) {
            ScrollLog("[%f] Controller equip event ended", *engineTime * 1000);
            _isControllerEquipEvent = false;
        }

        // Don't process frames while the game is paused
        auto ui = RE::UI::GetSingleton();
        if (!ui || ui->GameIsPaused()) {
            return;
        }

        if (_cancelCastQueued) {
            _cancelCastQueued = false;
            CancelCasting();
            return;
        }

        // Cancel the draw weapons wait after too much time has passed
        _drawWeaponLock.lock();
        if (
            _waitingDrawWeapons == AnimationWaitingState::Waiting &&
            *engineTime - _drawWeaponsStartTime > WeaponDrawTimeout
        ) {
            _waitingDrawWeapons = AnimationWaitingState::None;
            _drawWeaponLock.unlock();
            CancelCasting(true);
            return;
        }
        _drawWeaponLock.unlock();

        if (!_currentSpell.initialized) {
            if (_currentSpell.finalized) {
                if (_currentSpell.aftercastDelayFinished == AnimationWaitingState::Ready) {
                    RuntimeCastLog("Aftercast delay finished");
                    _currentSpell.finalized = false;
                }
                else if (
                    _currentSpell.aftercastDelayFinished == AnimationWaitingState::Waiting &&
                    *engineTime - _currentSpell.holdStart > RitualAftercastDelayTimeout
                ) {
                    _currentSpell.aftercastDelayFinished = AnimationWaitingState::Ready;
                }
                else if (*engineTime - _currentSpell.holdStart > AftercastDelayTimeout) {
                    RuntimeCastLog("Aftercast delay exceeded waiting time, cancelling");
                    _currentSpell.aftercastDelayFinished = AnimationWaitingState::Ready;
                    _currentSpell.finalized = false;
                    _queuedSpell = {};
                    RestoreEquipment();
                    return;
                }

                // The current spell finalized, trigger any queued spell
                if (_queuedSpell.initialized) {

                    // Unless it's a left handed spell and the bumper isn't enabled yet
                    {
                        std::lock_guard lock(_drawWeaponLock);
                        if (
                            _waitingDrawWeapons != AnimationWaitingState::None &&
                            _queuedSpell.castingSource == SkillCastSource::LeftHand
                        ) {
                            return;
                        }
                    }

                    // Or it represents the same spell but the aftercast delay hasn't finished
                    if (
                        !_currentSpell.isPower &&
                        !_queuedSpell.isPower &&
                        _currentSpell.spell == _queuedSpell.spell &&
                        _currentSpell.castingSource == GetRequestActualCastingSource(_queuedSpell) &&
                        _currentSpell.finalized
                    ) {
                        return;
                    }

                    bool casted = CastQueuedSpell();

                    // If there wasn't any queued spell to cast, restore
                    // the initial equipment if any
                    if (!casted) {
                        if (RestoreEquipment()) {
                            _currentSpell.finalized = false;
                        }
                    }
                    else {
                        std::lock_guard lock(_drawWeaponLock);
                        _waitingDrawWeapons = AnimationWaitingState::None;
                    }
                }
                else {
                    if (RestoreEquipment()) {
                        _currentSpell.finalized = false;
                    }
                }
            }
            return;
        }

        // While waiting for the staff windup, don't perform any other actions
        if (_currentSpell.startTime > *engineTime) {
            return;
        }

        // When bumper waiting finished, start the spell cast
        if (_currentSpell.waitingDisableBumper == AnimationWaitingState::Ready) {
            RuntimeCastLog("Bumper is ready, starting cast");
            
            // Send the attack key and reset the start time
            _currentSpell.waitingDisableBumper = AnimationWaitingState::None;
            _currentSpell.keypressStart = *engineTime;
            _currentSpell.keypressStarted = true;
            SendAttackBlockButton(_currentSpell.castingSource, 1.0f, 0.0f);

            // For manually released spells, casting "starts" instantly
            if (_currentSpell.manuallyReleased) {
                _currentSpell.castingStarted = true;
            }
        }

        // If waiting for bumper to be disabled, there is no further processing to do here
        // as this is reported via an animation event
        if (_currentSpell.waitingDisableBumper == AnimationWaitingState::Waiting) {
            if (_currentSpell.keypressStart == 0.0f) {
                _currentSpell.keypressStart = *engineTime;
                return;
            }

            // Time out after WaitingTimeout if bumper isn't disabled by then
            float heldTime = *engineTime - _currentSpell.keypressStart;
            if (heldTime > WaitingTimeout) {
                RuntimeCastLog("Spell bumper disabled wait time exceeded expected time, releasing.");
                _currentSpell = {};
                _queuedSpell = {};
                return;
            }
            return;
        }

        // If waiting on weapons to draw, check the state now
        if (_currentSpell.waitingReadyWeapons) {
            if (_currentSpell.keypressStart == 0.0f) {
                _currentSpell.keypressStart = *engineTime;
                return;
            }

            // Time out after WaitingTimeout if weapons aren't drawn by then
            float heldTime = *engineTime - _currentSpell.keypressStart;
            if (heldTime > WaitingTimeout) {
                RuntimeCastLog("Spell weapons ready wait time exceeded expected time, releasing.");
                _currentSpell = {};
                _queuedSpell = {};
                return;
            }

            auto player = RE::PlayerCharacter::GetSingleton();
            if (!player) {
                return;
            }

            auto state = player->AsActorState();
            if (!state) {
                return;
            }

            if (
                state->GetWeaponState() == RE::WEAPON_STATE::kDrawn &&
                state->GetAttackState() == RE::ATTACK_STATE_ENUM::kNone
            ) {
                RuntimeCastLog("Weapons are ready, starting cast");
                
                // Send the attack key and reset the start time
                _currentSpell.waitingReadyWeapons = false;
                _currentSpell.keypressStart = *engineTime;
                SendAttackBlockButton(_currentSpell.castingSource, 1.0f, 0.0f);
                _currentSpell.keypressStarted = true;

                // For manually released spells, casting "starts" instantly
                if (_currentSpell.manuallyReleased) {
                    _currentSpell.castingStarted = true;
                }
            }
            return;
        }
        
        // If the current spell is manually released, just continually send
        // virtual events until the key is released
        if (_currentSpell.manuallyReleased && _currentSpell.casting) {
            if (_currentSpell.keyHeld) {
                float heldTime = *engineTime - _currentSpell.keypressStart;
                if (heldTime > 0.0f) {
                    SendAttackBlockButton(_currentSpell.castingSource, 1.0f, heldTime);
                }
            }
            else {
                FinishCasting();
                // If the spell is a power, finalize it instantly
                if (_currentSpell.isPower) {
                    _currentSpell.finalized = true;
                    _currentSpell.initialized = false;
                }
            }
            return;
        }

        // If the spell is not casting but has not finalized, wait
        // until the player is no longer casting the spell to finalize it
        if (!_currentSpell.casting) {
            if (!_currentSpell.finalized) {
                auto *player = RE::PlayerCharacter::GetSingleton();
                if (!player) {
                    return;
                }

                if (!player->IsCasting(_currentSpell.spell)) {
                    _currentSpell.finalized = true;
                    _currentSpell.initialized = false;
                    _fulfilledSpell = { _currentSpell.spell, *engineTime, _currentSpell.castingSource };
                }
            }
            return;
        }
        
        if (_currentSpell.release) {
            FinishCasting();
            return;
        }

        // While casting, continually update the held time
        float heldTime = *engineTime - _currentSpell.keypressStart;
        if (heldTime > 0.0f) {
            SendAttackBlockButton(_currentSpell.castingSource, 1.0f, heldTime);
        }

        // If not casting, there is nothing to process
        if (!_currentSpell.castingStarted) {
            // If casting still hasn't started within WaitingTimeout of pressing the
            // button it was likely activated at an improper time and the button should
            // be released so it doesn't block further input
            if (heldTime > WaitingTimeout) {
                RuntimeCastLog("Spell cast wait time exceeded expected time, releasing.");
                FinishCasting();
                _currentSpell.finalized = true;
                _currentSpell.initialized = false;

                _queuedSpell = {};
            }
            return;
        }

        float castTime = (*engineTime - _currentSpell.holdStart);

        // For spells cast via hands, read the ready state from the caster
        if (!_currentSpell.isPower) {
            auto magicSource =
                _currentSpell.castingSource == SkillCastSource::RightHand ? 
                RE::MagicSystem::CastingSource::kRightHand :
                RE::MagicSystem::CastingSource::kLeftHand;
            auto player = RE::PlayerCharacter::GetSingleton();
            if (!player) {
                return;
            }

            auto magicCaster = player->GetMagicCaster(magicSource);
            if (!magicCaster) {
                return;
            }

            if (magicCaster->state.underlying() == 3 /*kReady*/) {
                _currentSpell.release = true;
                _currentSpell.castingStarted = false;
            }
            else if (castTime > (WaitingTimeout + _currentSpell.chargeTime)) {
                // Also release if the spell is not ready after WaitTime past its charge time
                _currentSpell.release = true;
                _currentSpell.castingStarted = false;
            }
        }
        // Once the caster reaches the ready state, fire the spell
        else if (castTime > _currentSpell.chargeTime) {
            // It takes one additional frame for the spell to be actually charged
            // once the charge time has been reached, so just queue the current
            // spell for releasing on the next frame
            _currentSpell.release = true;
            _currentSpell.castingStarted = false;
        }
    }
    
    RE::BSEventNotifyControl SpellCastController::ProcessEvent(const RE::BSAnimationGraphEvent *event, RE::BSTEventSource<RE::BSAnimationGraphEvent> *) {
        AnimationLog("Received animation event with tag %s", event->tag.c_str());

        // When the player gets staggered, cancel any in progress cast
        if (event->tag == "staggerStart"sv) {
            CancelCasting();
        }

        // When the InterruptCast event starts, cancel casting
        if (event->tag == "InterruptCast"sv && _currentSpell.initialized && _currentSpell.keypressStarted && !_currentSpell.isPower) {
            CancelCasting();
        }

        // Immediately after the spellfire event fires for a scroll, skyrim will consume
        // the scroll and change the equipment, so this is marked as a controller event
        // to prevent this from overwriting the stored equipment
        auto isRightSpellFireEvent = event->tag == "MRh_SpellFire_Event"sv;
        auto isLeftSpellFireEvent = event->tag == "MLh_SpellFire_Event"sv;

        if (isLeftSpellFireEvent || isRightSpellFireEvent) {
            if (_currentSpell.isScroll) {
                auto controller = this;
                _isControllerEquipEvent = true;

                // Because of the weird weapon swapping, queuing should be disabled for scrolls
                _queuedSpell = {};

                const auto scrollID = _currentSpell.spell->GetFormID();

                _fulfilledSpell = { _currentSpell.spell, *engineTime, _currentSpell.castingSource };

                // Additionally, as soon as possible after the scroll is consumed, restore the original equipment
                SKSE::GetTaskInterface()->AddTask([controller, scrollID]() {
                    ScrollLog("[%f] Scroll casting finished, ignoring next equip event", *engineTime * 1000);
                    controller->_isControllerEquipEvent = true;

                    controller->_currentSpell.aftercastDelayFinished = AnimationWaitingState::Ready;
                    controller->_currentSpell.finalized = true;
                    controller->_currentSpell.initialized = false;

                    controller->RestoreEquipment();

                    // Finally, it seems like using scrolls only triggers the container change event
                    // only when the last scroll in a stack is used, so the count must be manually
                    // rechecked after casting
                    SkillHUD::SharedHUD()->UpdateScrollCount(scrollID);
                });
            }
            return RE::BSEventNotifyControl::kContinue;
        }

        auto isDisableBumperEvent = event->tag == "DisableBumper"sv;
        auto isEnableBumperEvent = event->tag == "EnableBumper"sv;
        auto isBumperEvent = isDisableBumperEvent || isEnableBumperEvent; 

        _drawWeaponLock.lock();
        if (_waitingDrawWeapons == AnimationWaitingState::Waiting && event->tag == "attackStop"sv) {
            RuntimeCastLog("Controller bumper enabled");
            _waitingDrawWeapons = AnimationWaitingState::Ready;
        }

        if (_waitingDrawWeapons == AnimationWaitingState::Ready && event->tag == "End"sv) {
            _waitingDrawWeapons = AnimationWaitingState::None;
            
            // If there was a queued left hand spell, cast it now
            if (_queuedSpell.initialized && _queuedSpell.castingSource == SkillCastSource::LeftHand) {
                auto request = _queuedSpell;
                _queuedSpell = {};

                _drawWeaponLock.unlock();
                QueueRequest(request, false);
                return RE::BSEventNotifyControl::kContinue;
            }
        }

        _drawWeaponLock.unlock();
        
        // If not casting, or the current spell is manually released, there is nothing to process
        if ((!_currentSpell.initialized && !_currentSpell.finalized) || (_currentSpell.manuallyReleased && _currentSpell.waitingDisableBumper == AnimationWaitingState::None)) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto *player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return RE::BSEventNotifyControl::kContinue;
        }

        bool finalizedLeft = false;
        bool finalizedRight = false;
        bool started = false;
        const auto &tag = event->tag;

        switch (_currentSpell.castingSource) {
            case SkillCastSource::LeftHand:
                if (tag == "BeginCastLeft"sv) {
                    started = true;
                }
                break;
            case SkillCastSource::RightHand:
                if (tag == "BeginCastRight"sv) {
                    started = true;
                }
                break;
            case SkillCastSource::BothHands:
                // Dual casting appears to trigger left-hand events
                if (tag == "BeginCastLeft"sv) {
                    started = true;
                }
                break;
        }

        // The following animation events indicate that the aftercast delay finished
        // for a spell
        if (tag == "MRh_Equipped_Event" || tag == "MRh_WinStart" || tag == "MRh_WinEnd") {
            finalizedRight = true;
        }
        else if (tag == "MLh_Equipped_Event" || tag == "MLh_WinStart" || tag == "MLh_WinEnd") {
            // These are also used for dual casts
            finalizedLeft = true;
        }

        // Ritual spells use their own aftercast tag
        bool isRitualOutEvent = false;
        if (tag == "RitualSpellOut") {
            isRitualOutEvent = true;
        }

        if (_currentSpell.aftercastDelayFinished == AnimationWaitingState::None) {
            switch (_currentSpell.castingSource) {
                case SkillCastSource::LeftHand:
                    if (finalizedLeft) {
                        _currentSpell.aftercastDelayFinished = AnimationWaitingState::Ready;
                    }
                    break;
                case SkillCastSource::RightHand:
                    if (finalizedRight) {
                        _currentSpell.aftercastDelayFinished = AnimationWaitingState::Ready;
                    }
                    break;
                case SkillCastSource::BothHands:
                    // Ritual spells don't seem to have a consistent animation event that indicates,
                    // their aftercast delay is over, so an intermediate state is enabled on the
                    // spell for a timeout to clear the aftercast delay
                    if (isRitualOutEvent) {
                        _currentSpell.aftercastDelayFinished = AnimationWaitingState::Waiting;
                        _currentSpell.holdStart = *engineTime;
                    }
                    // Other kinds of dual casting trigger left-hand events
                    else if (finalizedLeft) {
                        _currentSpell.aftercastDelayFinished = AnimationWaitingState::Ready;
                    }
                    break;
            }
        }

        if (_currentSpell.waitingDisableBumper == AnimationWaitingState::Waiting && isBumperEvent) {
            // Right and dual casts can begin from either of the bumper events, but it seems
            // that left hand needs to specifically wait for enable bumper
            if (_currentSpell.castingSource == SkillCastSource::LeftHand && isDisableBumperEvent) {
                return RE::BSEventNotifyControl::kContinue;
            }
            RuntimeCastLog("Bumper disabled");
            _currentSpell.waitingDisableBumper = AnimationWaitingState::Ready;
            return RE::BSEventNotifyControl::kContinue;
        }

        if (started && _currentSpell.casting && player->IsCasting(_currentSpell.spell)) {
            _currentSpell.holdStart = *engineTime;
            _currentSpell.castingStarted = true;
        }

        return RE::BSEventNotifyControl::kContinue;
    }
    
    RE::BSEventNotifyControl SpellCastController::ProcessEvent(const RE::TESEquipEvent *event, RE::BSTEventSource<RE::TESEquipEvent> *) {
        if (_isControllerEquipEvent) {
            return RE::BSEventNotifyControl::kContinue;
        }
        else {
            auto player = RE::PlayerCharacter::GetSingleton();
            if (event->actor.get() != player) {
                return RE::BSEventNotifyControl::kContinue;
            }

            // Don't react to non-hand equipment
            auto object = event->baseObject;
            auto form = RE::TESForm::LookupByID(object);
            if (!form) {
                return RE::BSEventNotifyControl::kContinue;
            }

            switch (form->GetFormType()) {
                case RE::FormType::Weapon:
                    [[fallthrough]];
                case RE::FormType::Spell:
                    [[fallthrough]];
                case RE::FormType::Scroll:
                    // These items are always processed
                    break;
                case RE::FormType::Armor: {
                    // Armor must be a shield to count
                    auto armor = form->As<RE::TESObjectARMO>();
                    if (armor->equipSlot == GetLeftHandSlot()) {
                        break;
                    }
                    else {
                        return RE::BSEventNotifyControl::kContinue;
                    }
                }
                default:
                    return RE::BSEventNotifyControl::kContinue;
                    
            }

            // Whenever the user changes equipment, cancel any in-progress cast
            // as changing equipment cancels casting
            {
                std::lock_guard lock(_storedEquipmentLock);

                if (_storedEquipment) {
                    
                    AutocastLog("Cleared stored equipment due to an equip event");
                    delete _storedEquipment;
                    _storedEquipment = nullptr;
                }
            }

            ScrollLog("[%f] Equipped object of kind %d, with ID %x", *engineTime * 1000, form->GetFormType(), form->GetFormID());

            RuntimeCastLog("Cancelling cast due to equip event on hand slot");
            _cancelCastQueued = true;
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    
    RE::BSEventNotifyControl SpellCastController::ProcessEvent(const RE::TESSwitchRaceCompleteEvent *event, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent> *) {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!event || !event->subject || !player) {
            return RE::BSEventNotifyControl::kContinue;
        }

        if (event->subject.get() != player) {    
            return RE::BSEventNotifyControl::kContinue;
        }

        // Whenever the user changes race, cancel any in-progress cast
        // and wipe away any stored equipment that the new race might not be
        // able to equip properly
        {
            std::lock_guard lock(_storedEquipmentLock);

            if (_storedEquipment) {

                AutocastLog("Cleared stored equipment for race switch");
                delete _storedEquipment;
                _storedEquipment = nullptr;
            }
        }
        
        CancelCasting();

        return RE::BSEventNotifyControl::kContinue;
    }

    // MARK: Configuration

    void SpellCastController::AddSpellOverride(RE::FormID formID, SpellOverrides override) {
        _spellOverrideMap.emplace(formID, override);
    }

    void SpellCastController::ResetState() {
        {
            std::lock_guard lock(_storedEquipmentLock);
            if (_storedEquipment) {

                AutocastLog("Cleared stored equipment because state was reset");
                delete _storedEquipment;
                _storedEquipment = nullptr;
            } 
        }
        
        {
            std::lock_guard lock(_drawWeaponLock);
            _waitingDrawWeapons = AnimationWaitingState::None;
            _cancelCastQueued = false;
        }

        CancelCasting();
    }

    SkillCastBehaviour SpellCastController::GetSkillCastBehaviour() {
        return _configuration.castBehaviour;
    }

    void SpellCastController::SetSkillCastBehaviour(SkillCastBehaviour behaviour) {
        _configuration.castBehaviour = behaviour;
    }

    PotionSelectBehaviour SpellCastController::GetPotionSelectBehaviour() {
        return _configuration.potionBehaviour;
    }

    void SpellCastController::SetPotionSelectBehaviour(PotionSelectBehaviour behaviour) {
        _configuration.potionBehaviour = behaviour;
    }

    bool SpellCastController::GetAutoCastsEquippedSpell() {
        return _configuration.autoCastsEquippedSpell;
    }

    void SpellCastController::SetAutoCastsEquippedSpell(bool enabled) {
        _configuration.autoCastsEquippedSpell = enabled;
    }

    bool SpellCastController::GetPrefersDualCast() {
        return _configuration.prefersDualCast;
    }

    void SpellCastController::SetPrefersDualCast(bool enabled) {
        _configuration.prefersDualCast = enabled;
    }
    
    
    std::unordered_map<RE::FormID, SpellOverrides> SpellCastController::_spellOverrideMap;
    SpellCastController *SpellCastController::sharedController = nullptr;
    
    #ifdef AnimationSkips
        uint32_t SpellCastController::skipsAnimationStep = 0;
    #endif
}