#include "PersistenceController.h"
#include "ConfigurationController.h"

namespace AttunementSkillbar {

    void PersistenceController::Install() {
        auto coder = SKSE::GetSerializationInterface();
        coder->SetUniqueID(PluginID);
        coder->SetSaveCallback(Save);
        coder->SetLoadCallback(Load);
        coder->SetRevertCallback(Revert);
    }

    void PersistenceController::WriteLocalFormConfiguration(SKSE::SerializationInterface *coder, SkillSlotLocalFormConfiguration config) {
        coder->WriteRecordData(config.localID);
        coder->WriteRecordData(config.pluginNameLength);
        if (config.pluginNameLength) {
            coder->WriteRecordData(config.pluginName, config.pluginNameLength);
        }
    }

    SkillSlotLocalFormConfiguration PersistenceController::ReadLocalFormConfiguration(SKSE::SerializationInterface *coder) {
        SkillSlotLocalFormConfiguration config;
        coder->ReadRecordData(config.localID);
        coder->ReadRecordData(config.pluginNameLength);
        if (config.pluginNameLength) {
            config.pluginName = new char[config.pluginNameLength];
            coder->ReadRecordData(config.pluginName, config.pluginNameLength);
        }
        else {
            config.pluginName = nullptr;
        }

        return config;
    }

    void PersistenceController::Save(SKSE::SerializationInterface *coder) {
        // Acquire the configuration from the various entities
        PersistenceData data;
        data.behaviourConfiguration = SpellCastController::SharedController()->_configuration;
        data.inputConfiguration = InputEventController::SharedController()->_configuration;
        data.HUDConfiguration = SkillHUD::SharedHUD()->_configuration;
        data.activePotion = SkillHUD::SharedHUD()->_activePotionSlot;
        data.showHideConfiguration = SkillHUDPresentationController::SharedController()->_configuration;

        uint32_t attunementCount = data.HUDConfiguration.attunementCount;
        uint32_t skillCount = data.HUDConfiguration.skillSlotCount;

        SkillbarKeybind *attunementKeybinds = new SkillbarKeybind[attunementCount];
        SkillbarKeybind *skillKeybinds = new SkillbarKeybind[skillCount];
        AttunementSlotConfiguration *attunements = new AttunementSlotConfiguration[attunementCount];
        SkillSlotConfiguration *skills = new SkillSlotConfiguration[attunementCount * skillCount];
        SkillSlotConfiguration *potionSkills = new SkillSlotConfiguration[skillCount];
        PotionSlotConfiguration *potions = new PotionSlotConfiguration[skillCount];

        for (uint32_t i = 0; i < attunementCount; i++) {
            attunementKeybinds[i] = InputEventController::SharedController()->_attunementKeybinds[i];
            attunements[i] = SkillHUD::SharedHUD()->_attunementSlots[i]->_configuration;

            for (uint32_t j = 0; j < skillCount; j++) {
                skills[(i * skillCount) + j] = SkillHUD::SharedHUD()->_attunementSlots[i]->_skillSlots[j]->_configuration;
            }
        }

        for (uint32_t i = 0; i < skillCount; i++) {
            skillKeybinds[i] = InputEventController::SharedController()->_skillKeybinds[i];

            auto potion = static_cast<PotionSlot *>(SkillHUD::SharedHUD()->_potionAttunement->_skillSlots[i]);
            potionSkills[i] = potion->_configuration;
            potions[i] = potion->_genericConfiguration;
        }

        // Then write the data
        if (!coder->OpenRecord(RecordKind, 1)) {
            delete attunementKeybinds;
            delete skillKeybinds;
            delete attunements;
            delete skills;
            delete potionSkills;
            delete potions;
            return;
        }
        coder->WriteRecordData(data);

        for (uint32_t i = 0; i < attunementCount; i++) {
            coder->WriteRecordData(attunementKeybinds[i]);
            coder->WriteRecordData(attunements[i]);

            for (uint32_t j = 0; j < skillCount; j++) {
                coder->WriteRecordData(skills[(i * skillCount) + j]);
            }
        }

        for (uint32_t i = 0; i < skillCount; i++) {
            coder->WriteRecordData(skillKeybinds[i]);

            coder->WriteRecordData(potionSkills[i]);
            coder->WriteRecordData(potions[i]);
        }
        
        delete attunementKeybinds;
        delete skillKeybinds;
        delete attunements;
        delete skills;
        delete potionSkills;
        delete potions;

        // Write the cooldown data
        if (!coder->OpenRecord(CooldownRecordKind, 1)) {
            return;
        }

        auto powerCooldowns = SkillHUD::SharedHUD()->_powerCooldownMap;
        coder->WriteRecordData(SkillHUD::SharedHUD()->_shoutCooldownTotalTime);
        coder->WriteRecordData((uint32_t)powerCooldowns.size());
        
        for (auto i = powerCooldowns.begin(); i != powerCooldowns.end(); i++) {
            coder->WriteRecordData(i->first);
            coder->WriteRecordData(i->second);
        }

        // Write local form descriptions of skill/potion forms
        if (!coder->OpenRecord(LocalFormRecordKind, 1)) {
            return;
        }

        // Local config for the skill slots
        coder->WriteRecordData((uint32_t)(skillCount * attunementCount));
        for (uint32_t i = 0; i < attunementCount; i++) {
            for (uint32_t j = 0; j < skillCount; j++) {
                auto slot = SkillHUD::SharedHUD()->_attunementSlots[i]->_skillSlots[j];
                auto config = slot->GetLocalFormConfiguration();

                WriteLocalFormConfiguration(coder, config);
            }
        }

        // Local config for the potion slots
        coder->WriteRecordData((uint32_t)(2 * skillCount));
        for (uint32_t i = 0; i < skillCount; i++) {
            auto slot = static_cast<PotionSlot *>(SkillHUD::SharedHUD()->_potionAttunement->_skillSlots[i]);
            auto config = slot->GetLocalFormConfiguration();
            auto genericConfig = slot->GetGenericLocalFormConfiguration();
            
            WriteLocalFormConfiguration(coder, config);
            WriteLocalFormConfiguration(coder, genericConfig);
        }
    }

    void PersistenceController::Load(SKSE::SerializationInterface *coder) {
        SkillSlotLocalFormConfiguration *skillConfigs = nullptr;
        SkillSlotLocalFormConfiguration *potionConfigs = nullptr;

        uint32_t kind, version, length;
        while (coder->GetNextRecordInfo(kind, version, length)) {
            if (version != 1) {
                continue;
            }

            switch (kind) {
                case RecordKind:
                    LoadConfiguration(coder);
                    continue;
                case CooldownRecordKind:
                    LoadCooldowns(coder);
                    continue;
                case LocalFormRecordKind:
                    LoadLocalFormConfigurations(coder, &skillConfigs, &potionConfigs);
                    continue;
                default:
                    continue;
            }
        }

        // If local configs were saved and loaded, apply them
        if (skillConfigs) {
            auto HUDConfig = SkillHUD::SharedHUD()->_configuration;
            for (uint32_t i = 0; i < HUDConfig.attunementCount; i++) {
                auto attunement = SkillHUD::SharedHUD()->_attunementSlots[i];
                for (uint32_t j = 0; j < HUDConfig.skillSlotCount; j++) {
                    auto slot = attunement->_skillSlots[j];

                    auto config = skillConfigs[j + i * HUDConfig.skillSlotCount];
                    if (config.localID && config.pluginName) {
                        slot->SetLocalFormConfiguration(config);
                    }

                    if (config.pluginName) {
                        delete config.pluginName;
                    }
                }
            }

            delete skillConfigs;
        }

        if (potionConfigs) {
            auto HUDConfig = SkillHUD::SharedHUD()->_configuration;
            for (uint32_t i = 0; i < HUDConfig.skillSlotCount; i++) {
                auto slot = static_cast<PotionSlot *>(SkillHUD::SharedHUD()->_potionAttunement->_skillSlots[i]);

                auto config = potionConfigs[i * 2];
                auto genericConfig = potionConfigs[i * 2 + 1];
                if ((config.localID && config.pluginName) || (genericConfig.localID && genericConfig.pluginName)) {
                    slot->SetGenericLocalFormConfiguration(config, genericConfig);
                }

                if (config.pluginName) {
                    delete config.pluginName;
                }

                if (genericConfig.pluginName) {
                    delete genericConfig.pluginName;
                }
            }

            delete potionConfigs;
        }
    }

    void PersistenceController::LoadConfiguration(SKSE::SerializationInterface *coder) {
        PersistenceData data;
        coder->ReadRecordData(data);

        uint32_t attunementCount = data.HUDConfiguration.attunementCount;
        uint32_t skillCount = data.HUDConfiguration.skillSlotCount;

        SkillbarKeybind *attunementKeybinds = new SkillbarKeybind[attunementCount];
        SkillbarKeybind *skillKeybinds = new SkillbarKeybind[skillCount];
        AttunementSlotConfiguration *attunements = new AttunementSlotConfiguration[attunementCount];
        SkillSlotConfiguration *skills = new SkillSlotConfiguration[attunementCount * skillCount];
        SkillSlotConfiguration *potionSkills = new SkillSlotConfiguration[skillCount];
        PotionSlotConfiguration *potions = new PotionSlotConfiguration[skillCount];

        for (uint32_t i = 0; i < attunementCount; i++) {
            coder->ReadRecordData(attunementKeybinds[i]);
            coder->ReadRecordData(attunements[i]);

            for (uint32_t j = 0; j < skillCount; j++) {
                coder->ReadRecordData(skills[(i * skillCount) + j]);
            }
        }

        for (uint32_t i = 0; i < skillCount; i++) {
            coder->ReadRecordData(skillKeybinds[i]);

            coder->ReadRecordData(potionSkills[i]);
            coder->ReadRecordData(potions[i]);
        }

        // After loading the data, apply it
        ConfigurationController::SetAttunementCount(nullptr, attunementCount);
        ConfigurationController::SetSkillCount(nullptr, skillCount);

        // UI Configuration
        SkillHUD::SharedHUD()->SetLayoutConfiguration(data.HUDConfiguration);
        SkillHUD::SharedHUD()->SetActivePotion(data.activePotion);

        // Behaviour configuration
        ConfigurationController::SetSkillCastBehaviour(nullptr, static_cast<int32_t>(data.behaviourConfiguration.castBehaviour));
        ConfigurationController::SetPotionSelectBehaviour(nullptr, static_cast<int32_t>(data.behaviourConfiguration.potionBehaviour));
        ConfigurationController::SetAutoCastsEquippedSpell(nullptr, data.behaviourConfiguration.autoCastsEquippedSpell);
        ConfigurationController::SetPrefersDualCast(nullptr, data.behaviourConfiguration.prefersDualCast);

        // Input configuration
        ConfigurationController::SetAssignRightHandKeyID(nullptr, data.inputConfiguration.assignRightHandKeyID);
        ConfigurationController::SetAssignLeftHandKeyID(nullptr, data.inputConfiguration.assignLeftHandKeyID);
        ConfigurationController::SetAssignDualCastModifierKeyID(nullptr, data.inputConfiguration.assignDualCastModifierKeyID);
        ConfigurationController::SetAssignmentModifierKeyID(nullptr, data.inputConfiguration.assignmentModifierKeyID);
        ConfigurationController::SetPotionKeyID(nullptr, data.inputConfiguration.potionKeyID);
        ConfigurationController::SetNextAttunementKeyID(nullptr, data.inputConfiguration.nextAttunementKeyID);
        ConfigurationController::SetPreviousAttunementKeyID(nullptr, data.inputConfiguration.previousAttunementKeyID);

        // Show/hide configuration
        ConfigurationController::SetShowsAlways(nullptr, data.showHideConfiguration.showsAlways);
        ConfigurationController::SetShowsInCombat(nullptr, data.showHideConfiguration.showsInCombat);
        ConfigurationController::SetShowsWeaponsDrawn(nullptr, data.showHideConfiguration.showsWeaponsDrawn);

        // Skill and attunement assignments and keybinds
        for (uint32_t i = 0; i < attunementCount; i++) {
            ConfigurationController::SetKeybindForAttunement(nullptr, i, attunementKeybinds[i].keyID);
            ConfigurationController::SetTextureIDForAttunement(nullptr, i, attunements[i].textureID - AttunementTextureOffset);

            for (uint32_t j = 0; j < skillCount; j++) {
                SkillHUD::SharedHUD()->AssignSkillForAttunement(skills[(i * skillCount) + j], i, j);
            }
        }

        for (uint32_t i = 0; i < skillCount; i++) {
            ConfigurationController::SetKeybindForSkill(nullptr, i, skillKeybinds[i].keyID);

            SkillHUD::SharedHUD()->AssignPotion(potionSkills[i], potions[i], i);
        }

        delete attunementKeybinds;
        delete skillKeybinds;
        delete attunements;
        delete skills;
        delete potionSkills;
        delete potions;
    }

    void PersistenceController::LoadCooldowns(SKSE::SerializationInterface *coder) {
        float shoutCooldown;
        coder->ReadRecordData(shoutCooldown);

        uint32_t powerCount;
        coder->ReadRecordData(powerCount);

        std::unordered_map<RE::FormID, SkillCooldown> powerCooldowns;
        for (uint32_t i = 0; i < powerCount; i++) {
            RE::FormID formID;
            SkillCooldown cooldown;

            coder->ReadRecordData(formID);
            coder->ReadRecordData(cooldown);

            powerCooldowns.emplace(formID, cooldown);
        }

        SkillHUD::SharedHUD()->SetCooldownData(shoutCooldown, powerCooldowns);
    }

    void PersistenceController::LoadLocalFormConfigurations(
        SKSE::SerializationInterface *coder,
        SkillSlotLocalFormConfiguration **skillConfigs,
        SkillSlotLocalFormConfiguration **potionConfigs
    ) {
        uint32_t totalSkillCount;
        coder->ReadRecordData(totalSkillCount);
        *skillConfigs = new SkillSlotLocalFormConfiguration[totalSkillCount];

        for (uint32_t i = 0; i < totalSkillCount; i++) {
            (*skillConfigs)[i] = ReadLocalFormConfiguration(coder);
        }

        uint32_t totalPotionCount;
        coder->ReadRecordData(totalPotionCount);
        *potionConfigs = new SkillSlotLocalFormConfiguration[totalPotionCount];

        for (uint32_t i = 0; i < totalPotionCount; i++) {
            (*potionConfigs)[i] = ReadLocalFormConfiguration(coder);
        }

    }

    void PersistenceController::Revert(SKSE::SerializationInterface *) {
        // Ask everything to reset their state
        InputEventController::SharedController()->ResetState();
        SpellCastController::SharedController()->ResetState();
        SkillHUD::SharedHUD()->ResetState();

        // Then apply the default configuration
        PersistenceData defaultData {};

        uint32_t attunementCount = defaultData.HUDConfiguration.attunementCount;
        uint32_t skillCount = defaultData.HUDConfiguration.skillSlotCount;
        
        ConfigurationController::SetAttunementCount(nullptr, attunementCount);
        ConfigurationController::SetSkillCount(nullptr, skillCount);

        // UI Configuration
        SkillHUD::SharedHUD()->SetLayoutConfiguration(defaultData.HUDConfiguration);

        // Behaviour configuration
        ConfigurationController::SetSkillCastBehaviour(nullptr, static_cast<int32_t>(defaultData.behaviourConfiguration.castBehaviour));
        ConfigurationController::SetPotionSelectBehaviour(nullptr, static_cast<int32_t>(defaultData.behaviourConfiguration.potionBehaviour));
        ConfigurationController::SetAutoCastsEquippedSpell(nullptr, defaultData.behaviourConfiguration.autoCastsEquippedSpell);
        ConfigurationController::SetPrefersDualCast(nullptr, defaultData.behaviourConfiguration.prefersDualCast);

        // Input configuration
        ConfigurationController::SetAssignRightHandKeyID(nullptr, defaultData.inputConfiguration.assignRightHandKeyID);
        ConfigurationController::SetAssignLeftHandKeyID(nullptr, defaultData.inputConfiguration.assignLeftHandKeyID);
        ConfigurationController::SetAssignDualCastModifierKeyID(nullptr, defaultData.inputConfiguration.assignDualCastModifierKeyID);
        ConfigurationController::SetAssignmentModifierKeyID(nullptr, defaultData.inputConfiguration.assignmentModifierKeyID);
        ConfigurationController::SetPotionKeyID(nullptr, defaultData.inputConfiguration.potionKeyID);
        ConfigurationController::SetNextAttunementKeyID(nullptr, defaultData.inputConfiguration.nextAttunementKeyID);
        ConfigurationController::SetPreviousAttunementKeyID(nullptr, defaultData.inputConfiguration.previousAttunementKeyID);

        // Show/hide configuration
        ConfigurationController::SetShowsAlways(nullptr, defaultData.showHideConfiguration.showsAlways);
        ConfigurationController::SetShowsInCombat(nullptr, defaultData.showHideConfiguration.showsInCombat);
        ConfigurationController::SetShowsWeaponsDrawn(nullptr, defaultData.showHideConfiguration.showsWeaponsDrawn);

        // Skill and attunement assignments and keybinds
        for (uint32_t i = 0; i < attunementCount; i++) {
            ConfigurationController::SetKeybindForAttunement(nullptr, i, RE::BSKeyboardDevice::Keys::kF1 + i);
            ConfigurationController::SetTextureIDForAttunement(nullptr, i, i);

            for (uint32_t j = 0; j < skillCount; j++) {
                SkillHUD::SharedHUD()->AssignSkillForAttunement({}, i, j);
            }
        }

        for (uint32_t i = 0; i < skillCount; i++) {
            ConfigurationController::SetKeybindForSkill(nullptr, i, RE::BSKeyboardDevice::Keys::kNum1 + i);

            SkillHUD::SharedHUD()->AssignPotion({}, {}, i);
        }
    }
}