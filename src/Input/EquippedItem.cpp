#include "EquippedItem.h"
#include "SpellCastController.h"

// #define EquipmentDebug
#ifdef EquipmentDebug
#define EquipmentLog(...) RE::ConsoleLog::GetSingleton()->Print(__VA_ARGS__)
#else
#define EquipmentLog(...)
#endif

#pragma push_macro("windows")
#undef GetObject

namespace AttunementSkillbar {

    StoredEquipment::StoredEquipment() {
        _leftHand = EquippedItem::CurrentlyEquipped(EquippedItemHand::LeftHand);
        _rightHand = EquippedItem::CurrentlyEquipped(EquippedItemHand::RightHand);
    }

    StoredEquipment::~StoredEquipment() {
        delete _leftHand;
        delete _rightHand;
    }

    bool StoredEquipment::ContainsTwoHandedItem() {
        return _rightHand->IsTwoHanded();
    }

    bool StoredEquipment::ContainsStaffItem() {
        return _rightHand->IsStaff() || _leftHand->IsStaff();
    }

    bool StoredEquipment::ContainsWeaponMainHand() {
        return _rightHand->GetKind() == EquippedItemKind::WeaponArmor;
    }

    RE::TESForm *StoredEquipment::SpellItemInHand(EquippedItemHand hand) {
        auto item = hand == EquippedItemHand::LeftHand ? _leftHand : _rightHand;
        switch (item->_kind) {
            case EquippedItemKind::Spell:
                return static_cast<SpellEquippedItem *>(item)->GetSpell();
                break;
            case EquippedItemKind::Scroll:
                return static_cast<ScrollEquippedItem *>(item)->GetScroll();
                break;
            case EquippedItemKind::Null:
                break;
            default:
                return nullptr;
        }

        // If the left hand is requested but nulled, determine if the right hand
        // has a two handed spell item and return it instead
        if (hand == EquippedItemHand::LeftHand) {
            auto rightHandSpell = SpellItemInHand(EquippedItemHand::RightHand);
            if (!rightHandSpell) {
                return nullptr;
            }

            switch (rightHandSpell->GetFormType()) {
                case RE::FormType::Spell:
                    if (rightHandSpell->As<RE::SpellItem>()->IsTwoHanded()) {
                        return rightHandSpell;
                    }
                    break;
                case RE::FormType::Scroll:
                    if (rightHandSpell->As<RE::ScrollItem>()->IsTwoHanded()) {
                        return rightHandSpell;
                    }
                    break;
            }
        }

        return nullptr;
    }

    bool StoredEquipment::Equip(bool *leftHandChangedOut, bool *rightHandChangedOut) {
        bool leftHandChanged = _leftHand->Equip();
        bool rightHandChanged = _rightHand->Equip();

        if (leftHandChangedOut) {
            *leftHandChangedOut = leftHandChanged;
        }
        if (rightHandChangedOut) {
            *rightHandChangedOut = rightHandChanged;
        }

        if (_rightHand->GetKind() == EquippedItemKind::WeaponArmor) {
            return rightHandChanged && !_rightHand->IsTwoHanded();
        }
        return false;
    }

    EquippedItem *EquippedItem::CurrentlyEquipped(EquippedItemHand hand) {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return new NullEquippedItem();
        }

        bool leftHand = hand == EquippedItemHand::LeftHand;
        RE::TESForm *form = player->GetEquippedObject(leftHand);
        if (!form) {
            auto item = new FistEquippedItem();
            item->_hand = hand;
            return item;
        }

        RE::FormID formID = form->formID;

        bool isWeapon = false;
        switch (form->GetFormType()) {
            case RE::FormType::Spell: {
                auto spell = form->As<RE::SpellItem>();
                if (!spell) {
                    return new NullEquippedItem();
                }

                // Two handed spells only need to be equipped in the right hand
                // therefore there is no action to take for the left hand
                if (spell->IsTwoHanded() && leftHand) {
                    return new NullEquippedItem();
                }

                auto item = new SpellEquippedItem(formID, spell->IsTwoHanded());
                item->_hand = hand;
                return item;
            }
            case RE::FormType::Scroll: {
                auto scroll = form->As<RE::ScrollItem>();
                if (!scroll) {
                    return new NullEquippedItem();
                }

                // Two handed scrolls only need to be equipped in the right hand
                // therefore there is no action to take for the left hand
                if (scroll->IsTwoHanded() && leftHand) {
                    return new NullEquippedItem();
                }

                auto item = new ScrollEquippedItem(formID, scroll->IsTwoHanded());
                item->_hand = hand;
                return item;
            }
            // For armor and weapons attempt to get the unique ID of the specific instance
            case RE::FormType::Armor:
                break;
            case RE::FormType::Weapon:
                isWeapon = true;
                break;
            default:
                return new NullEquippedItem();
        }

        
        bool isTwoHanded = false;
        bool isStaff = false;

        if (isWeapon) {
            auto weapon = form->As<RE::TESObjectWEAP>();
            if (!weapon) {
                EquipmentLog("Created null item because it's not castable to weapon");
                return new NullEquippedItem();
            }

            switch (weapon->GetWeaponType()) {
                case RE::WEAPON_TYPE::kBow:
                    [[fallthrough]];
                case RE::WEAPON_TYPE::kCrossbow:
                    [[fallthrough]];
                case RE::WEAPON_TYPE::kTwoHandAxe:
                    [[fallthrough]];
                case RE::WEAPON_TYPE::kTwoHandSword:
                    // If the weapon is two handed, it only needs to be equipped
                    // in the right hand, so there doesn't need to be any action
                    // taken for the left hand
                    if (leftHand) {
                        return new NullEquippedItem();
                    }
                    isTwoHanded = true;
                    break;
                case RE::WEAPON_TYPE::kStaff:
                    isStaff = true;
                    break;
            }
        }

        RE::InventoryEntryData *equippedData = nullptr;
        if (isWeapon) {
            equippedData = player->GetEquippedEntryData(leftHand);
        }
        else {
            // For armors, the equipped entry data must be found in the inventory
            auto changes = player->GetInventoryChanges();
            if (!changes) {
                // Fall back to the basic object if the unique ID can't be obtained/created
                auto boundObject = new WeaponArmorEquippedItem(formID, 0, isTwoHanded, isStaff);
                boundObject->_hand = hand;
                return boundObject;
            }
            for (auto entry : *changes->entryList) {
                if (entry->GetObject() == form) {
                    equippedData = entry;
                    break;
                }
            }
        }
        if (!equippedData) {
            EquipmentLog("Creating null item because there's no equipped data");
            return new NullEquippedItem();
        }

        auto object = equippedData->GetObject();
        if (!object) {
            auto item = new FistEquippedItem();
            item->_hand = hand;
            return item;
        }

        uint16_t uniqueID = 0;
        for (auto &extraList : *equippedData->extraLists) {
            auto wornDataType = hand == EquippedItemHand::LeftHand ? RE::ExtraDataType::kWornLeft : RE::ExtraDataType::kWorn;
            if (!extraList->HasType(wornDataType)) {
                continue;
            }
            if (extraList->HasType(RE::ExtraDataType::kUniqueID)) {
                auto* uniqueIDExtra = extraList->GetByType<RE::ExtraUniqueID>();
                if (uniqueIDExtra) {
                    uniqueID = uniqueIDExtra->uniqueID;
                    break;
                }
            }
            else {
                // If the item doesn't have an extra ID create one for it
                auto changes = player->GetInventoryChanges();
                if (!changes) {
                    continue;
                }
                uint16_t nextID = changes->GetNextUniqueID();
		        auto extraID = new RE::ExtraUniqueID(object->formID, nextID);
                extraList->Add(extraID);
                uniqueID = nextID;
                EquipmentLog("Created unique ID list for item %u",(unsigned)uniqueID);
            }
        }

        EquipmentLog("Stored item with unique ID %u",(unsigned)uniqueID);
        auto boundObject = new WeaponArmorEquippedItem(object->formID, uniqueID, isTwoHanded, isStaff);
        boundObject->_hand = hand;
        return boundObject;
    }

    RE::BGSEquipSlot *EquippedItem::GetSlot() {
        return _hand == EquippedItemHand::LeftHand ?
            SpellCastController::GetLeftHandSlot() :
            SpellCastController::GetRightHandSlot();
    }

    bool FistEquippedItem::Equip() {
        auto currentItem = EquippedItem::CurrentlyEquipped(_hand);
        if (currentItem->GetKind() == _kind) {
            delete currentItem;
            return false;
        }
        delete currentItem;

        EquipmentLog("Restoring equipped fist in hand %d", _hand);
        auto player = RE::PlayerCharacter::GetSingleton();
        auto equipmentManager = RE::ActorEquipManager::GetSingleton();
        if (!player || !equipmentManager) {
            return false;
        }

        // Equip then unequip a dummy dagger
        auto form = RE::TESForm::LookupByID<RE::TESForm>(0x00020163);
        if (!form) {
            return false;
        }
        auto dummyDagger = form->As<RE::TESObjectWEAP>();
        if (!dummyDagger) {
            return false;
        }
        equipmentManager->EquipObject(player, dummyDagger, nullptr, 1, GetSlot(), false, true, false);
        equipmentManager->UnequipObject(player, dummyDagger, nullptr, 1, GetSlot(), false, true, false);

        return true;
    }

    bool WeaponArmorEquippedItem::Equip() {
        auto currentItem = EquippedItem::CurrentlyEquipped(_hand);
        if (currentItem->GetKind() == _kind) {
            auto currentWeapon = static_cast<WeaponArmorEquippedItem *>(currentItem);
            if (currentWeapon->_formID == _formID && currentWeapon->_uniqueID == _uniqueID) {
                delete currentItem;
                return false;
            }
        }
        delete currentItem;

        EquipmentLog("Restoring equipped weapon/armor in hand %d", _hand);
        auto player = RE::PlayerCharacter::GetSingleton();
        auto equipmentManager = RE::ActorEquipManager::GetSingleton();
        if (!player || !equipmentManager) {
            return false;
        }

        auto form = RE::TESForm::LookupByID(_formID);
        if (!form) {
            return false;
        }

        RE::TESBoundObject *object = nullptr;

        switch (form->GetFormType()) {
            case RE::FormType::Weapon: {
                object = form->As<RE::TESObjectWEAP>();
                break;
            }
            case RE::FormType::Armor: {
                object = form->As<RE::TESObjectARMO>();
                break;
            }
            default:
                return false;
        }

        if (!object) {
            return false;
        }

        // If the item wasn't equipped with a matching unique ID and there is none set,
        // equip the generic item
        if (_uniqueID) {
            try {
                // Ensure the player still has an instance of the weapon or armor in their inventory
                RE::TESObjectREFR::InventoryItemMap inventoryMap = player->GetInventory();
                
                std::unique_ptr<RE::InventoryEntryData> *entryData = nullptr;
                for (auto& [boundObject, data] : inventoryMap) {
                    if (boundObject->formID == object->GetFormID()) {
                        entryData = &data.second;
                    }
                }

                if (!entryData || !*entryData || !(*entryData)->extraLists) {
                    return false;
                }

                for (auto* extraList : *(*entryData)->extraLists) {
                    if (!extraList) {
                        continue;
                    }

                    if (extraList->HasType(RE::ExtraDataType::kUniqueID)) {
                        if (_uniqueID == extraList->GetByType<RE::ExtraUniqueID>()->uniqueID) {
                            equipmentManager->EquipObject(player, object, extraList, 1, GetSlot());
                            return true;
                        }
                    }
                }

                return false;
            }
            catch (std::exception &e) {
		        logger::error("An error occurred while attempting to re-equip weapon/armor: {}", e.what());
            }
        }
        else {
            equipmentManager->EquipObject(player, object, nullptr, 1, GetSlot());
        }

        return true;
    }

    bool ScrollEquippedItem::Equip() {
        auto currentItem = EquippedItem::CurrentlyEquipped(_hand);
        if (currentItem->GetKind() == _kind) {
            auto currentScroll = static_cast<ScrollEquippedItem *>(currentItem);
            if (currentScroll->_formID == _formID) {
                delete currentItem;
                return false;
            }
        }
        delete currentItem;

        EquipmentLog("Restoring equipped scroll in hand %d", _hand);
        auto player = RE::PlayerCharacter::GetSingleton();
        auto equipmentManager = RE::ActorEquipManager::GetSingleton();
        if (!player || !equipmentManager) {
            return false;
        }

        auto form = RE::TESForm::LookupByID(_formID);
        if (!form || form->formType != RE::FormType::Scroll) {
            return false;
        }
        auto scroll = form->As<RE::ScrollItem>();

        // Ensure the player still has an instance of the scroll in their inventory
        RE::TESObjectREFR::InventoryItemMap inventoryMap = player->GetInventory();
        if (!(inventoryMap.contains(scroll) && inventoryMap[scroll].first > 0)) {
            return false;
        }

        equipmentManager->EquipObject(player, scroll, nullptr, 1, GetSlot());
        return true;
    }

    bool SpellEquippedItem::Equip() {
        auto currentItem = EquippedItem::CurrentlyEquipped(_hand);
        if (currentItem->GetKind() == _kind) {
            auto currentSpell = static_cast<SpellEquippedItem *>(currentItem);
            if (currentSpell->_formID == _formID) {
                delete currentItem;
                return false;
            }
        }
        delete currentItem;

        EquipmentLog("Restoring equipped spell in hand %d", _hand);
        auto player = RE::PlayerCharacter::GetSingleton();
        auto equipmentManager = RE::ActorEquipManager::GetSingleton();
        if (!player || !equipmentManager) {
            return false;
        }

        auto form = RE::TESForm::LookupByID(_formID);
        if (!form || form->formType != RE::FormType::Spell) {
            return false;
        }
        auto spell = form->As<RE::SpellItem>();

        // Ensure the player still knows the spell
        if (!player->HasSpell(spell)) {
            return false;
        }

        equipmentManager->EquipSpell(player, spell, GetSlot());
        return true;
    }

}

#pragma pop_macro("windows")