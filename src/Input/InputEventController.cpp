#include "InputEventController.h"
#include "SpellCastController.h"
#include "../SkillHUD/SkillHUD.h"
#include "../SkillHUD/SkillAssignmentHUD.h"
#include <SimpleIni.h>

#pragma push_macro("windows")
#undef GetObject

namespace AttunementSkillbar {

    // MARK: Input blocking states

    void InputEventController::SetGameLoading(bool loading) {
        _gameLoading = loading;
    }

    bool InputEventController::IsTransformed() {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return true;
        }

        return player->GetPlayerRuntimeData().unkBD0 != 0;
    }

    bool InputEventController::IsPlayerInteractionEnabled() {
        if (_gameLoading) {
            return false;
        }

        auto player = RE::PlayerCharacter::GetSingleton();
        auto VATS = RE::VATS::GetSingleton();
        auto controls = RE::PlayerControls::GetSingleton();
        auto UI = RE::UI::GetSingleton();

        if (!UI || !controls || !player || !VATS || !player->Is3DLoaded()) {
            return false;
        }

        // Interaction is always disabled while the loading menu or dialogue menus are open
        if (UI->IsMenuOpen(RE::LoadingMenu::MENU_NAME)) {
            return false;
        }
        else if (UI->IsMenuOpen(RE::FaderMenu::MENU_NAME)) {
            return false;
        }
        
        if (UI->IsMenuOpen(RE::DialogueMenu::MENU_NAME)) {
            return false;
        }

        auto scene = player->GetCurrentScene();
        bool sceneIsPlaying = !!scene;
        if (sceneIsPlaying) {
            uint32_t field = scene->unkB0;
            sceneIsPlaying = (bool)(field >> 24);
        }

        auto enabled = 
            // Disabled when player input is explicitly blocked
            !controls->blockPlayerInput &&
            // Disabled while a cutscene is running (e.g. helgen)
            !sceneIsPlaying &&
            // Disabled while player is ragdolled, dead or in kill cam
            !player->IsInRagdollState() &&
            !player->IsDead() &&
            VATS->VATSMode != RE::VATS::VATS_MODE::kKillCam;

        if (enabled) {
            auto flags = player->GetActorRuntimeData().boolFlags;
            enabled = enabled &&
                !flags.any(RE::Actor::BOOL_FLAGS::kCastingDisabled) &&
                !flags.any(RE::Actor::BOOL_FLAGS::kAttackingDisabled) &&
                !flags.any(RE::Actor::BOOL_FLAGS::kMovementBlocked);
        }

        return enabled;
    }

    // MARK: Input disabling hook

    void InputEventController::InstallHooks() {
        auto &trampoline = SKSE::GetTrampoline();
        REL::Relocation<uintptr_t> caller { RELOCATION_ID(67315, 68617) };
        _baseDispatchInputEvent = trampoline.write_call<5>(caller.address() + RELOCATION_OFFSET(0x7B, 0x7B), DispatchInputEvent);
    }

    void InputEventController::DispatchInputEvent(RE::BSTEventSource<RE::InputEvent *> *eventSource, RE::InputEvent **event) {
        // Any events containing a spell cast interrupt keydown should cancel the
        // current spellcast first before the game gets a chance to process it.
        // Additionally, spell cast controller is notified of all button down/up events
        // including during menus, so that potential casts are properly released when
        // resuming the game from pause after releasing a held key
        bool handled = false;
        if (auto *ui = RE::UI::GetSingleton(); ui && !_inputDisabled) {
            auto inputController = InputEventController::SharedController();
            for (auto *e = *event; e; e = e->next) {
                if (e->eventType != RE::INPUT_EVENT_TYPE::kButton) {
                    continue;
                }

                auto *button = e->AsButtonEvent();
                if (!button) {
                    continue;
                }
                
                if (ui->IsMenuOpen(RE::MagicMenu::MENU_NAME)) {
                    // If the magic menu handler took ownership of this event, tell
                    // the game to not process further
                    auto eventHandled = InputEventController::SharedController()->HandleMagicMenuButtonEvent(button);
                    if (eventHandled) {
                        handled = true;
                    }
                }
                else if (ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME)) {
                    // If the inventory menu handler took ownership of this event, tell
                    // the game to not process further
                    auto eventHandled = InputEventController::SharedController()->HandleInventoryMenuButtonEvent(button);
                    if (eventHandled) {
                        handled = true;
                    }
                }

                if (button->IsDown()) {
                    if (!ui->GameIsPaused() && !ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME)) {
                        unsigned hand = 0;
                        uint32_t keyID = 0;
                        bool restoreEquipment = true;
                        if (inputController->IsSpellCancellingButton(button, hand, keyID, restoreEquipment)) {
                            SpellCastController::SharedController()->CancelOrQueueCasting(hand, keyID, restoreEquipment);
                            continue;
                        }
                    }

                    SpellCastController::SharedController()->KeyPressed(GetDeviceIndependentIDCode(button));
                }
                else if (button->IsUp()) {
                    if (inputController->_potionKeyHeld && inputController->IsPotionKey(button)) {
                        inputController->_potionKeyHeld = false;
                        SkillHUD::SharedHUD()->ReleasePotionIndicator();

                        if (inputController->_potionHUDActive) {
                            inputController->_potionHUDActive = false;
                            SkillHUD::SharedHUD()->SetShowsPotions(false, true);
                        }
                        else if (!inputController->_potionSwapped && !ui->GameIsPaused()) {
                            // If this is released with the game not being paused, before the potion
                            // selection UI is displayed and without another potion being selected
                            // consume the potion in the main slot
                            auto HUD = SkillHUD::SharedHUD();
                            auto index = HUD->GetActivePotion();
                            SkillSlotSkillConfiguration skill;
                            PotionSlotConfiguration potion;
                            HUD->InitializePotionConfigurationWithIndex(index, skill, potion);

                            SpellCastController::SharedController()->UseConsumable(skill.skillID, skill.castingSource, potion);
                        }

                        inputController->_potionSwapped = false;

                    }

                    auto skillSlotIndex = inputController->GetSkillSlotIndexForEvent(button);
                    if (skillSlotIndex != -1) {
                        SkillHUD::SharedHUD()->ReleaseSkillAtIndex(skillSlotIndex);
                    }

                    SpellCastController::SharedController()->KeyReleased(GetDeviceIndependentIDCode(button));
                }
            }
        }

        if (handled) {
            return;
        }
        
        static RE::InputEvent *dummy[] = { nullptr };
        if (!event) {
            _baseDispatchInputEvent(eventSource, event);
            return;
        }

        if (_inputDisabled) {
            InputEventController::SharedController()->ProcessAssignmentEvent(event, eventSource);
        }
        else {
            _baseDispatchInputEvent(eventSource, event);
        }
    }
    
    void InputEventController::ShowSkillAssignmentHUD() {
        auto kind = InputEventController::SharedController()->_potionAssignmentKind;
        SkillAssignmentHUD::SharedHUD()->Open(kind);
        _inputDisabled = true;
    }

    void InputEventController::SkillAssignmentFinished() {
        _inputDisabled = false;
    }

    // MARK: Input processing

    uint32_t InputEventController::GetDeviceIndependentIDCode(RE::ButtonEvent *event) {
        // In C++, the ID value depends on the device, but in MCM the
        // device is derived from the ID code and only directly matches
        // the native code for keyboard keys
        auto IDCode = event->GetIDCode();
        return GetDeviceIndependentIDCode(IDCode, event->GetDevice());
    }
    
    uint32_t InputEventController::GetDeviceIndependentIDCode(uint32_t IDCode, RE::INPUT_DEVICE device) {
        // In C++, the ID value depends on the device, but in MCM the
        // device is derived from the ID code and only directly matches
        // the native code for keyboard keys
        switch (device) {
            case RE::INPUT_DEVICE::kMouse:
                IDCode += 256;
                break;
            case RE::INPUT_DEVICE::kGamepad:
                IDCode += 266;
        }

        return IDCode;
    }

    RE::BSEventNotifyControl InputEventController::ProcessEvent(
        RE::InputEvent *const *event,
        RE::BSTEventSource<RE::InputEvent *> *
    ) {
        if (!event) {
            return RE::BSEventNotifyControl::kContinue;
        }

        // Don't process while transformed on non-controllable
        if (IsTransformed() || !IsPlayerInteractionEnabled()) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto result = RE::BSEventNotifyControl::kContinue;

        for (auto *e = *event; e; e = e->next) {
            if (e->eventType != RE::INPUT_EVENT_TYPE::kButton) {
                continue;
            }

            auto *button = e->AsButtonEvent();
            if (!button) {
                continue;
            }

            // Don't process these events while a game menu is open, except for the
            // magic and inventory menus
            if (auto *ui = RE::UI::GetSingleton(); ui && ui->GameIsPaused()) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (button->IsDown()) {
                if (IsPotionKey(button)) {
                    _potionKeyHeld = true;
                    SkillHUD::SharedHUD()->PressPotionIndicator();
                }

                // Attunement keys switch the attunement
                auto attunementIndex = GetAttunementSlotIndexForEvent(button);
                if (attunementIndex != -1) {
                    SkillHUD::SharedHUD()->ActivateAttunement(attunementIndex, true);
                    continue;
                }

                if (IsNextAttunementKey(button)) {
                    SkillHUD::SharedHUD()->ActivateNextAttunement(true);
                    continue;
                }

                if (IsPreviousAttunementKey(button)) {
                    SkillHUD::SharedHUD()->ActivatePreviousAttunement(true);
                    continue;
                }

                // Skill slot keys perform or queue the cast
                auto skillSlotIndex = GetSkillSlotIndexForEvent(button);
                if (skillSlotIndex != -1) {
                    SkillHUD::SharedHUD()->PressSkillAtIndex(skillSlotIndex, _potionKeyHeld);

                    if (!_potionKeyHeld) {
                        auto skillConfiguration = SkillHUD::SharedHUD()->GetSkillConfigurationAtIndex(skillSlotIndex);
                        SpellCastController::SharedController()->QueueSkill(
                            skillConfiguration.skillID,
                            skillConfiguration.castingSource,
                            button->GetIDCode()
                        );
                    }
                    else {
                        SkillSlotSkillConfiguration skill;
                        PotionSlotConfiguration potion;
                        SkillHUD::SharedHUD()->InitializePotionConfigurationWithIndex(skillSlotIndex, skill, potion);
                        SkillHUD::SharedHUD()->SetActivePotion(skillSlotIndex);

                        if (SpellCastController::SharedController()->GetPotionSelectBehaviour() == PotionSelectBehaviour::UseAndEquip) {
                            SpellCastController::SharedController()->UseConsumable(skill.skillID, skill.castingSource, potion);
                        }

                        _potionSwapped = true;
                    }
                    continue;
                }
            }
            else if (button->IsHeld()) {
                if (!_potionHUDActive && IsPotionKey(button) && button->heldDownSecs > PotionSwapHoldTime) {
                    _potionHUDActive = true;
                    SkillHUD::SharedHUD()->SetShowsPotions(true, true);
                } 
            }
            else if (button->IsUp()) {
                auto skillSlotIndex = GetSkillSlotIndexForEvent(button);
                if (skillSlotIndex != -1) {
                    SkillHUD::SharedHUD()->ReleaseSkillAtIndex(skillSlotIndex, _potionKeyHeld);
                }
            }
        }

        return result;
    }

    // MARK: Skill assignment events

    bool InputEventController::HandleMagicMenuButtonEvent(RE::ButtonEvent *event) {
        uint32_t ID = GetDeviceIndependentIDCode(event);
        bool handled = false;

        if (event->IsDown()) {
            // Test and store if this is one of the modifier keys
            if (ID == _configuration.assignmentModifierKeyID) {
                _assignmentModifierHeld = true;
            }
            else if (ID == _configuration.assignDualCastModifierKeyID) {
                _dualCastModifierHeld = true;
            }
        }
        else if (event->IsUp()) {
            // Test and store if this is one of the modifier keys
            if (ID == _configuration.assignmentModifierKeyID) {
                _assignmentModifierHeld = false;
            }
            else if (ID == _configuration.assignDualCastModifierKeyID) {
                _dualCastModifierHeld = false;
            }
            // If any assignment key is released, attempt to assign the highlighted spell
            else if (ID == _configuration.assignLeftHandKeyID) {
                BeginSpellAssignmentForHand(RE::MagicSystem::CastingSource::kLeftHand);
            }
            else if (ID == _configuration.assignRightHandKeyID) {
                BeginSpellAssignmentForHand(RE::MagicSystem::CastingSource::kRightHand);
            }
        }

        // Regardless of the event state, if it includes an assignment button while the
        // assignment modifier is held, ask the game to not further handle this event
        if (ID == _configuration.assignLeftHandKeyID && IsAssignmentModifierHeld()) {
            handled = true;
        }
        else if (ID == _configuration.assignRightHandKeyID && IsAssignmentModifierHeld()) {
            handled = true;
        }

        return handled;
    }

    inline bool InputEventController::IsAssignmentModifierHeld() {
        return _configuration.assignmentModifierKeyID == 0 || _assignmentModifierHeld;
    }

    void InputEventController::BeginSpellAssignmentForHand(RE::MagicSystem::CastingSource hand) {
        // If an assignment modifier is defined but not held, don't bring up the assignment menu
        if (!IsAssignmentModifierHeld()) {
            return;
        }

        // If the dual cast modifier is set and held, change the casting source to dual casting
        if (_configuration.assignDualCastModifierKeyID != 0 && _dualCastModifierHeld) {
            hand = RE::MagicSystem::CastingSource::kOther;
        }

        // Otherwise suspend interruptions and bring up the skillbar, asking the user to press the
        // skill slot they want to assign to, if a valid skill is highlighted

        auto *ui = RE::UI::GetSingleton();
        if (!ui) {
            return;
        }

        auto *magicMenu = static_cast<RE::MagicMenu*>(ui->GetMenu(RE::MagicMenu::MENU_NAME).get());
        if (!magicMenu) {
            return;
        }

        RE::GFxValue result;
        magicMenu->uiMovie->GetVariable(&result, "_root.Menu_mc.inventoryLists.itemList.selectedEntry.formId");
        
        if (result.GetType() != RE::GFxValue::ValueType::kNumber) {
            return;
        }

        RE::FormID formID = static_cast<std::uint32_t>(result.GetNumber());
        auto form = RE::TESForm::LookupByID(formID);
        if (!form) {
            return;
        }

        // The form must be a spell or shout to be assigned to the skillbar
        switch (form->GetFormType()) {
            case RE::FormType::Shout: {
                auto shout = form->As<RE::TESShout>();
                if (!shout) {
                    return;
                }

                _skillToAssign.skillID = formID;
                _skillToAssign.castingSource = RE::MagicSystem::CastingSource::kOther;
                break;
            }
            case RE::FormType::Spell: {
                auto spell = form->As<RE::SpellItem>();
                if (!spell) {
                    return;
                }

                auto source = hand;

                // Some spells specifically require the left hand; if they do, set the
                // hand to left regardless of the binding used
                if (spell->GetEquipSlot() == SpellCastController::GetLeftHandSlot()) {
                    source = RE::MagicSystem::CastingSource::kLeftHand;
                }

                // Powers and two handed skills don't need any specific hand
                if (spell->GetSpellType() != RE::MagicSystem::SpellType::kSpell || spell->IsTwoHanded()) {
                    source = RE::MagicSystem::CastingSource::kOther;
                }

                _skillToAssign.skillID = formID;
                _skillToAssign.castingSource = source;
                break;
            }
            default:
                return;
        }

        // If a valid assignment configuration was created, start spell asignment
        _assignmentModifierHeld = false;
        _dualCastModifierHeld = false;
        _potionKeyHeld = false;
        _potionHUDActive = false;
        SkillHUD::SharedHUD()->SetShowsPotions(false, true);
        _potionAssignmentKind = PotionAssignmentKind::None;
        ShowSkillAssignmentHUD();
    }

    
    RE::BSEventNotifyControl InputEventController::ProcessAssignmentEvent(
        RE::InputEvent *const *event,
        RE::BSTEventSource<RE::InputEvent *> *
    ) {
        if (!event) {
            return RE::BSEventNotifyControl::kContinue;
        }

        for (auto *e = *event; e; e = e->next) {
            if (e->eventType != RE::INPUT_EVENT_TYPE::kButton) {
                continue;
            }

            auto *button = e->AsButtonEvent();
            if (!button) {
                continue;
            }

            auto key = GetDeviceIndependentIDCode(button);
            if (button->IsUp()) {
                // Escape always dismisses the assignment and has no configuration
                if (key == RE::BSKeyboardDevice::Key::kEscape) {
                    SkillAssignmentHUD::SharedHUD()->Dismiss();
                    continue;
                }

                // Attunement keys switch the attunement as normal
                auto attunementIndex = GetAttunementSlotIndexForEvent(button);
                if (attunementIndex != -1) {
                    SkillHUD::SharedHUD()->ActivateAttunement(attunementIndex, true);
                    continue;
                }

                if (IsNextAttunementKey(button)) {
                    SkillHUD::SharedHUD()->ActivateNextAttunement(true);
                    continue;
                }

                if (IsPreviousAttunementKey(button)) {
                    SkillHUD::SharedHUD()->ActivatePreviousAttunement(true);
                    continue;
                }

                // Skill slot keys finalize the assignment
                auto skillSlotIndex = GetSkillSlotIndexForEvent(button);
                if (skillSlotIndex != -1) {
                    SkillAssignmentHUD::SharedHUD()->AssignSkill(_skillToAssign, _potionToAssign, skillSlotIndex);
                    continue;
                }
            }
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    
    // MARK: Potion assignment events

    bool InputEventController::HandleInventoryMenuButtonEvent(RE::ButtonEvent *event) {
        if (_configuration.potionKeyID == 0) {
            return false;
        }

        uint32_t ID = GetDeviceIndependentIDCode(event);
        bool handled = false;

        if (event->IsDown()) {
            // Test and store if this is one of the modifier keys
            if (ID == _configuration.assignmentModifierKeyID) {
                _assignmentModifierHeld = true;
            }
            else if (ID == _configuration.assignDualCastModifierKeyID) {
                _dualCastModifierHeld = true;
            }
        }
        else if (event->IsUp()) {
            // Test and store if this is one of the modifier keys
            if (ID == _configuration.assignmentModifierKeyID) {
                _assignmentModifierHeld = false;
            }
            else if (ID == _configuration.assignDualCastModifierKeyID) {
                _dualCastModifierHeld = false;
            }
            // If any assignment key is released, attempt to assign the highlighted potion
            else if (ID == _configuration.assignLeftHandKeyID) {
                BeginPotionAssignment(PotionAssignmentKind::Specific);
            }
            else if (ID == _configuration.assignRightHandKeyID) {
                BeginPotionAssignment(PotionAssignmentKind::Generic);
            }
        }

        // Regardless of the event state, if it includes an assignment button while the
        // assignment modifier is held, ask the game to not further handle this event
        if (ID == _configuration.assignLeftHandKeyID && IsAssignmentModifierHeld()) {
            handled = true;
        }
        else if (ID == _configuration.assignRightHandKeyID && IsAssignmentModifierHeld()) {
            handled = true;
        }

        return handled;
    }

    void InputEventController::BeginPotionAssignment(PotionAssignmentKind kind) {
        // If an assignment modifier is defined but not held, don't bring up the assignment menu
        if (!IsAssignmentModifierHeld()) {
            return;
        }

        // Otherwise suspend interruptions and bring up the skillbar, asking the user to press the
        // potion slot they want to assign to, if a valid potion is highlighted

        auto *ui = RE::UI::GetSingleton();
        if (!ui) {
            return;
        }

        auto *inventoryMenu = static_cast<RE::InventoryMenu*>(ui->GetMenu(RE::InventoryMenu::MENU_NAME).get());
        if (!inventoryMenu) {
            return;
        }

        auto *items = inventoryMenu->itemList;
        if (!items) {
            return;
        }

        auto *selectedItem = items->GetSelectedItem();
        if (!selectedItem) {
            return;
        }

        auto *object = selectedItem->data.objDesc;
        if (!object) {
            return;
        }

        auto *boundObject = object->GetObject();
        if (!boundObject) {
            return;
        }

        bool isPotion = false;
        bool isScroll = false;
        if (boundObject->GetFormType() == RE::FormType::AlchemyItem) {
            isPotion = true;
        }
        else if (boundObject->GetFormType() == RE::FormType::Scroll) {
            isScroll = true;
        }

        if (isPotion) {
            _potionAssignmentKind = kind;
            _skillToAssign = { boundObject->GetFormID() };
            if (kind == PotionAssignmentKind::Generic) {
                _skillToAssign.skillID = 0;
            }
            _potionToAssign = PotionConfigurationWithAlchemyItem(boundObject->As<RE::AlchemyItem>());
        }
        else if (isScroll) {
            _potionAssignmentKind = PotionAssignmentKind::Specific;

            auto scroll = boundObject->As<RE::ScrollItem>();

            auto hand = (kind == PotionAssignmentKind::Generic) ? 
                RE::MagicSystem::CastingSource::kRightHand :
                RE::MagicSystem::CastingSource::kLeftHand;

            _skillToAssign = {
                scroll->GetFormID(),
                (scroll->IsTwoHanded() ? RE::MagicSystem::CastingSource::kOther : hand)
            };
            _potionToAssign = PotionConfigurationWithScrollItem(scroll);
        }
        else {
            return;
        }

        // If a valid assignment configuration was created, start spell asignment
        _assignmentModifierHeld = false;
        _dualCastModifierHeld = false;
        _potionKeyHeld = false;
        _potionHUDActive = false;
        SkillHUD::SharedHUD()->SetShowsPotions(false, true);
        ShowSkillAssignmentHUD();
    }

    PotionSlotConfiguration InputEventController::PotionConfigurationWithScrollItem(RE::ScrollItem *) {
        return { RE::ActorValue::kNone, 0, PotionSlotItemKind::Scroll };
    }

    PotionSlotConfiguration InputEventController::PotionConfigurationWithAlchemyItem(RE::AlchemyItem *item) {
        if (
            item->IsFood() ||
            item->data.flags.any(RE::AlchemyItem::AlchemyFlag::kFoodItem) ||
            item->HasKeywordString("VendorItemFood")
        ) {
            // Consider an item to be alcohol if it has the restore stamina food effect
            bool isAlcohol = false;
            for (const auto &effect : item->effects) {
                switch (effect->baseEffect->GetFormID()) {
                    case 0xF33CC:
                    case 0x3EB16:
                        isAlcohol = true;
                    default:
                        continue;
                }
            }

            if (isAlcohol) {
                return { RE::ActorValue::kNone, 0, PotionSlotItemKind::AlcoholDrink };
            }
            else if (item->HasKeywordString("VendorItemFoodRaw")) {
                // This doesn't fully catch ALL raw items, but seems like the best option
                return { RE::ActorValue::kNone, 0, PotionSlotItemKind::RawFood };
            }
            return { RE::ActorValue::kNone, 0, PotionSlotItemKind::CookedFood };
        }

        bool isPoison =
            item->IsPoison() ||
            item->data.flags.any(RE::AlchemyItem::AlchemyFlag::kPoison) ||
            item->HasKeywordString("VendorItemPoison");

        auto kind = isPoison ? PotionSlotItemKind::Poison : PotionSlotItemKind::Potion;

        const RE::EffectSetting *effect = item->GetCostliestEffectItem()->baseEffect;
        RE::ActorValue actorValue = effect->GetMagickSkill();
        if (actorValue == RE::ActorValue::kNone) {
            actorValue = effect->data.primaryAV;
        }

        // Coalesce some effects into a single one
        switch (actorValue) {
            case RE::ActorValue::kHealRateMult:
            case RE::ActorValue::kHealRate:
                actorValue = RE::ActorValue::kHealth;
                break;
            case RE::ActorValue::kStaminaRateMult:
            case RE::ActorValue::kStaminaRate:
                actorValue = RE::ActorValue::kStamina;
                break;
            case RE::ActorValue::kMagickaRateMult:
            case RE::ActorValue::kMagickaRate:
                actorValue = RE::ActorValue::kMagicka;
                break;
        }

        return { actorValue, effect->GetFormID(), kind };
    }

    float InputEventController::HungerMagnitudeForFoodItem(RE::AlchemyItem *item) {
        auto player = RE::PlayerCharacter::GetSingleton();

        for (auto effect : item->effects) {
            auto baseForm = effect->baseEffect->GetLocalFormID();
            // NOTE: mods may modify these values, but this should be fine as long as
            // the tiers remain the same
            switch (baseForm) {
                case 0x2EE4:
                    return 380.0f;
                case 0x2EE3:
                    return 220.0f;
                case 0x2EE2:
                    return 18.0f;
                case 0x2EE1:
                    return 2.0f;
                case 0xE831:
                    if (player && player->HasKeywordString("Vampire")) {
                        return 380.0f;
                    }
                    else {
                        return 0.01f;
                    }
            }
        }

        // Always default to 0.01f so that the magnitude is enough to consume the item
        return 0.01f;
    }

    float InputEventController::PotionMagnitudeForConfiguration(RE::AlchemyItem *item, PotionSlotConfiguration config) {
        if (
            item->IsFood() ||
            item->data.flags.any(RE::AlchemyItem::AlchemyFlag::kFoodItem) ||
            item->HasKeywordString("VendorItemFood")
        ) {
            // Consider an item to be alcohol if it has the restore stamina food effect
            bool isAlcohol = false;
            for (const auto &effect : item->effects) {
                switch (effect->baseEffect->GetFormID()) {
                    case 0xF33CC:
                    case 0x3EB16:
                        isAlcohol = true;
                    default:
                        continue;
                }
            }

            bool isRawFood = !isAlcohol && item->HasKeywordString("VendorItemFoodRaw");

            if (isAlcohol && config.kind == PotionSlotItemKind::AlcoholDrink) {
                return HungerMagnitudeForFoodItem(item);
            }
            else if (isRawFood && config.kind == PotionSlotItemKind::RawFood) {
                return HungerMagnitudeForFoodItem(item);
            }
            else if (config.kind == PotionSlotItemKind::CookedFood && !isAlcohol && !isRawFood) {
                return HungerMagnitudeForFoodItem(item);
            }
            else {
                return 0.0f;
            }
        }

        bool isPoison =
            item->IsPoison() ||
            item->data.flags.any(RE::AlchemyItem::AlchemyFlag::kPoison) ||
            item->HasKeywordString("VendorItemPoison");

        if (isPoison && config.kind != PotionSlotItemKind::Poison) {
            return 0.0f;
        }
        else if (!isPoison && config.kind == PotionSlotItemKind::Poison) {
            return 0.0f;
        }

        // Loop through the effects to find the matching one
        for (auto alchemyEffect : item->effects) {
            auto effect = alchemyEffect->baseEffect;

            if (effect->GetFormID() == config.effectID) {
                return alchemyEffect->effectItem.magnitude;
            }
        }

        return 0.0f;
    }
    
    // MARK: Skill slot interactions

    int32_t InputEventController::GetAttunementSlotIndexForEvent(RE::ButtonEvent *event) {
        auto key = GetDeviceIndependentIDCode(event);
        if (key == 0) {
            return -1;
        }

        for (size_t i = 0; i < _configuration.attunementCount; i++) {
            if (_attunementKeybinds[i].keyID == key) {
                return (int32_t)i;
            }
        }

        return -1;
    }

    int32_t InputEventController::GetSkillSlotIndexForEvent(RE::ButtonEvent *event) {
        auto key = GetDeviceIndependentIDCode(event);
        if (key == 0) {
            return -1;
        }

        for (size_t i = 0; i < _configuration.skillSlotCount; i++) {
            if (_skillKeybinds[i].keyID == key) {
                return (int32_t)i;
            }
        }

        return -1;
    }

    bool InputEventController::IsPotionKey(RE::ButtonEvent *event) {
        auto key = GetDeviceIndependentIDCode(event);
        if (key == 0) {
            return false;
        }

        return key == _configuration.potionKeyID;
    }

    bool InputEventController::IsNextAttunementKey(RE::ButtonEvent *event) {
        auto key = GetDeviceIndependentIDCode(event);
        if (key == 0) {
            return false;
        }

        return key == _configuration.nextAttunementKeyID;
    }

    bool InputEventController::IsPreviousAttunementKey(RE::ButtonEvent *event) {
        auto key = GetDeviceIndependentIDCode(event);
        if (key == 0) {
            return false;
        }

        return key == _configuration.previousAttunementKeyID;
    }

    // MARK: Standard keybinds

    RE::BSEventNotifyControl InputEventController::ProcessEvent(
        const RE::MenuOpenCloseEvent *event,
        RE::BSTEventSource<RE::MenuOpenCloseEvent> *
    ) {
        if (!event || event->opening) {
            return RE::BSEventNotifyControl::kContinue;
        }

        if (event->menuName == "Journal Menu"sv) {
            ReloadKeybinds();
        }
        
        return RE::BSEventNotifyControl::kContinue;
    }

    void InputEventController::InitializeDodgeKey() {
        auto path = "Data\\SKSE\\Plugins\\TK Dodge RE.ini";
        CSimpleIniA ini;
        ini.SetUnicode();

        const auto result = ini.LoadFile(path);
        if (result < 0) {
            return;
        }

        _dodgeKey = ini.GetLongValue("Main", "DodgeHotkey", 0);
    }

    void InputEventController::ReloadKeybinds() {
        keybinds->clear();
        auto *userEvents = RE::UserEvents::GetSingleton();

        // The following keybinds must cancel any in-progress spell cast
        // and also clear out any queued spell
        CacheKeybindsForEvent(userEvents->leftAttack, (unsigned)SkillCastSource::LeftHand, true);
        CacheKeybindsForEvent(userEvents->rightAttack, (unsigned)SkillCastSource::RightHand, true);
        CacheKeybindsForEvent(userEvents->dualAttack, (unsigned)SkillCastSource::BothHands, true);
        CacheKeybindsForEvent(userEvents->jump, 0, false);
        CacheKeybindsForEvent(userEvents->readyWeapon, 0, true);

    }

    void InputEventController::CacheKeybindsForEvent(RE::BSFixedString &event, unsigned hand, bool restoresEquipment) {
        auto *controlMap = RE::ControlMap::GetSingleton();

        auto mouseKey = controlMap->GetMappedKey(event, RE::INPUT_DEVICE::kMouse);
        auto keyboardKey = controlMap->GetMappedKey(event, RE::INPUT_DEVICE::kKeyboard);
        auto gamepadKey = controlMap->GetMappedKey(event, RE::INPUT_DEVICE::kGamepad);

        if (mouseKey != 0xFF) {
            keybinds->push_back({event, mouseKey, RE::INPUT_DEVICE::kMouse, hand, restoresEquipment});
        }

        if (keyboardKey != 0xFF && keyboardKey != 0) {
            keybinds->push_back({event, keyboardKey, RE::INPUT_DEVICE::kKeyboard, hand, restoresEquipment});
        }

        if (gamepadKey != 0xFF && gamepadKey != 0) {
            keybinds->push_back({event, gamepadKey, RE::INPUT_DEVICE::kGamepad, hand, restoresEquipment});
        }
    }

    Keybind InputEventController::GetKeybindForEvent(const RE::BSFixedString &event) {
        for (const Keybind &keybind : *keybinds) {
            if (keybind.event == event) {
                return keybind;
            }
        }

        return {event, 0, RE::INPUT_DEVICE::kNone};
    }

    unsigned InputEventController::GetHandForButtonEvent(RE::ButtonEvent *button) {
        for (const Keybind &keybind : *keybinds) {
            if (keybind.device == button->device && keybind.keyID == button->idCode) {
                return keybind.attackHand;
            }
        }

        return 0;
    }

    bool InputEventController::IsSpellCancellingButton(RE::ButtonEvent *button, unsigned &hand, uint32_t &key, bool &restoresEquipment) {
        for (const Keybind &keybind : *keybinds) {
            if (keybind.device == button->device && keybind.keyID == button->idCode) {
                hand = keybind.attackHand;
                key = GetDeviceIndependentIDCode(keybind.keyID, keybind.device);
                restoresEquipment = keybind.restoresEquipment;
                return true;
            }
        }

        if (_dodgeKey && GetDeviceIndependentIDCode(button) == _dodgeKey) {
            hand = 0;
            key = _dodgeKey;
            restoresEquipment = false;
            return true;
        }

        return false;
    }

    // MARK: Configuration

    void InputEventController::ResetState() {
        _potionKeyHeld = false;
        _dualCastModifierHeld = false;
        _assignmentModifierHeld = false;
        _potionSwapped = false;
        _potionHUDActive = false;
        
        SkillAssignmentFinished();
        _inputDisabled = false;
    }

    void InputEventController::ReleaseSkillKeybinds() {
        if (_attunementKeybinds) {
            delete _attunementKeybinds;
            _attunementKeybinds = nullptr;
        }

        if (_skillKeybinds) {
            delete _skillKeybinds;
            _skillKeybinds = nullptr;
        }
    }

    void InputEventController::UseDefaultConfiguration() {
        ReleaseSkillKeybinds();
        _configuration = {};
        _configuration.attunementCount = 4;
        _configuration.skillSlotCount = 10;

        // Set up the default keybinds; users are expected to change them so
        // there is no point coming up with a complicated config file for these
        _attunementKeybinds = new SkillbarKeybind[4];
        _attunementKeybinds[0] = { RE::BSKeyboardDevice::Keys::kF1 };
        _attunementKeybinds[1] = { RE::BSKeyboardDevice::Keys::kF2 };
        _attunementKeybinds[2] = { RE::BSKeyboardDevice::Keys::kF3 };
        _attunementKeybinds[3] = { RE::BSKeyboardDevice::Keys::kF4 };

        _skillKeybinds = new SkillbarKeybind[10];
        _skillKeybinds[0] = { RE::BSKeyboardDevice::Keys::kNum1 };
        _skillKeybinds[1] = { RE::BSKeyboardDevice::Keys::kNum2 };
        _skillKeybinds[2] = { RE::BSKeyboardDevice::Keys::kNum3 };
        _skillKeybinds[3] = { RE::BSKeyboardDevice::Keys::kNum4 };
        _skillKeybinds[4] = { RE::BSKeyboardDevice::Keys::kNum5 };
        _skillKeybinds[5] = { RE::BSKeyboardDevice::Keys::kNum6 };
        _skillKeybinds[6] = { RE::BSKeyboardDevice::Keys::kNum7 };
        _skillKeybinds[7] = { RE::BSKeyboardDevice::Keys::kNum8 };
        _skillKeybinds[8] = { RE::BSKeyboardDevice::Keys::kNum9 };
        _skillKeybinds[9] = { RE::BSKeyboardDevice::Keys::kNum0 };
    }

    void InputEventController::SetConfiguration(
        InputEventControllerConfiguration configuration,
        SkillbarKeybind *attunementKeybinds,
        SkillbarKeybind *skillKeybinds
    ) {
        ReleaseSkillKeybinds();
        _configuration = configuration;

        // Copy over the keybinds defined in the provided configuration
        _attunementKeybinds = new SkillbarKeybind[configuration.attunementCount];
        _skillKeybinds = new SkillbarKeybind[configuration.skillSlotCount];

        for (size_t i = 0; i < configuration.attunementCount; i++) {
            _attunementKeybinds[i] = attunementKeybinds[i];
        }

        for (size_t i = 0; i < configuration.skillSlotCount; i++) {
            _skillKeybinds[i] = skillKeybinds[i];
        }

    }

    uint32_t InputEventController::GetAssignmentModifierKeyID() const {
        return _configuration.assignmentModifierKeyID;
    }

    void InputEventController::SetAssignmentModifierKeyID(uint32_t keyID) {
        _configuration.assignmentModifierKeyID = keyID;
    }

    // Assign right hand
    uint32_t InputEventController::GetAssignRightHandKeyID() const {
        return _configuration.assignRightHandKeyID;
    }

    void InputEventController::SetAssignRightHandKeyID(uint32_t keyID) {
        _configuration.assignRightHandKeyID = keyID;
    }

    // Assign left hand
    uint32_t InputEventController::GetAssignLeftHandKeyID() const {
        return _configuration.assignLeftHandKeyID;
    }

    void InputEventController::SetAssignLeftHandKeyID(uint32_t keyID) {
        _configuration.assignLeftHandKeyID = keyID;
    }

    // Dual-cast modifier
    uint32_t InputEventController::GetAssignDualCastModifierKeyID() const {
        return _configuration.assignDualCastModifierKeyID;
    }

    void InputEventController::SetAssignDualCastModifierKeyID(uint32_t keyID) {
        _configuration.assignDualCastModifierKeyID = keyID;
    }

    // Potion key
    uint32_t InputEventController::GetPotionKeyID() const {
        return _configuration.potionKeyID;
    }

    void InputEventController::SetPotionKeyID(uint32_t keyID) {
        // If this keybind is assigned anywhere else, remove it from there
        UnassignKeybind({ keyID });

        _configuration.potionKeyID = keyID;
        SkillHUD::SharedHUD()->AssignPotionKeybind(keyID);
    }

    // Attunement cycle keys
    
    uint32_t InputEventController::GetNextAttunementKeyID() const {
        return _configuration.nextAttunementKeyID;
    }

    void InputEventController::SetNextAttunementKeyID(uint32_t keyID) {
        // If this keybind is assigned anywhere else, remove it from there
        UnassignKeybind({ keyID });

        _configuration.nextAttunementKeyID = keyID;
        SkillHUD::SharedHUD()->AssignNextAttunementKeybind(keyID);
    }


    uint32_t InputEventController::GetPreviousAttunementKeyID() const {
        return _configuration.previousAttunementKeyID;
    }

    void InputEventController::SetPreviousAttunementKeyID(uint32_t keyID) {
        // If this keybind is assigned anywhere else, remove it from there
        UnassignKeybind({ keyID });

        _configuration.previousAttunementKeyID = keyID;
        SkillHUD::SharedHUD()->AssignPreviousAttunementKeybind(keyID);
    }
    
    uint32_t InputEventController::GetAttunementCount() {
        return _configuration.attunementCount;
    }

    void InputEventController::SetAttunementCount(uint32_t count) {
        if (count == _configuration.attunementCount) {
            return;
        }

        count = std::clamp(count, (uint32_t)1, MaxAttunementSlots);

        uint32_t oldCount = _configuration.attunementCount;
        auto oldKeybinds = _attunementKeybinds;

        _attunementKeybinds = new SkillbarKeybind[count];

        // Copy over as many old keybinds as will fit in the new configuration
        _configuration.attunementCount = count;
        for (size_t i = 0; i < count && i < oldCount; i++) {
            _attunementKeybinds[i] = oldKeybinds[i];
        }

        for (size_t i = oldCount; i < count; i++) {
            _attunementKeybinds[i] = { 0 };
        }
    }

    uint32_t InputEventController::GetSkillCount() {
        return _configuration.skillSlotCount;
    }

    void InputEventController::SetSkillCount(uint32_t count) {
        if (count == _configuration.skillSlotCount) {
            return;
        }

        count = std::clamp(count, (uint32_t)1, MaxAttunementSlots);

        uint32_t oldCount = _configuration.skillSlotCount;
        auto oldKeybinds = _skillKeybinds;

        _skillKeybinds = new SkillbarKeybind[count];

        // Copy over as many old keybinds as will fit in the new configuration
        _configuration.skillSlotCount = count;
        for (size_t i = 0; i < count && i < oldCount; i++) {
            _skillKeybinds[i] = oldKeybinds[i];
        }

        for (size_t i = oldCount; i < count; i++) {
            _skillKeybinds[i] = { 0 };
        }
    }

    void InputEventController::UnassignKeybind(SkillbarKeybind keybind) {
        // The "no keybind" keybind cannot be unassigned
        if (keybind.keyID == 0) {
            return;
        }

        for (uint32_t i = 0; i < _configuration.attunementCount; i++) {
            if (_attunementKeybinds[i] == keybind) {
                _attunementKeybinds[i] = {};
                SkillHUD::SharedHUD()->AssignAttunementKeybind(0, i);
            }
        }

        for (uint32_t i = 0; i < _configuration.skillSlotCount; i++) {
            if (_skillKeybinds[i] == keybind) {
                _skillKeybinds[i] = {};
                SkillHUD::SharedHUD()->AssignSkillKeybind(0, i);
            }
        }

        if (_configuration.potionKeyID == keybind.keyID) {
            _configuration.potionKeyID = 0;
            SkillHUD::SharedHUD()->AssignPotionKeybind(0);
        }

        if (_configuration.nextAttunementKeyID == keybind.keyID) {
            _configuration.nextAttunementKeyID = 0;
            SkillHUD::SharedHUD()->AssignNextAttunementKeybind(0);
        }

        if (_configuration.previousAttunementKeyID == keybind.keyID) {
            _configuration.previousAttunementKeyID = 0;
            SkillHUD::SharedHUD()->AssignPreviousAttunementKeybind(0);
        }
    }

    SkillbarKeybind InputEventController::GetKeybindForAttunement(int32_t index) {
        if (index < 0 || (uint32_t)index >= _configuration.attunementCount) {
            return {};
        }

        return _attunementKeybinds[index];
    }

    void InputEventController::SetKeybindForAttunement(int32_t index, SkillbarKeybind keybind) {
        if (index < 0 || (uint32_t)index >= _configuration.attunementCount) {
            return;
        }

        // If this keybind is assigned anywhere else, remove it from there
        UnassignKeybind(keybind);

        _attunementKeybinds[index] = keybind;
        SkillHUD::SharedHUD()->AssignAttunementKeybind(keybind.keyID, index);
    }


    SkillbarKeybind InputEventController::GetKeybindForSkill(int32_t index) {
        if (index < 0 || (uint32_t)index >= _configuration.skillSlotCount) {
            return {};
        }

        return _skillKeybinds[index];
    }

    void InputEventController::SetKeybindForSkill(int32_t index, SkillbarKeybind keybind) {
        if (index < 0 || (uint32_t)index >= _configuration.skillSlotCount) {
            return;
        }

        // If this keybind is assigned anywhere else, remove it from there
        UnassignKeybind(keybind);

        _skillKeybinds[index] = keybind;
        SkillHUD::SharedHUD()->AssignSkillKeybind(keybind.keyID, index);
    }

    InputEventController *InputEventController::_sharedController = nullptr;
    bool InputEventController::_inputDisabled = false;
    bool InputEventController::_gameLoading = false;
}


#pragma pop_macro("windows")