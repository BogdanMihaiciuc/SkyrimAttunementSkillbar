#pragma once

#include "../Input/InputEventController.h"
#include "../Input/SpellCastController.h"

#include "../SkillHUD/SkillHUD.h"
#include "../SkillHUD/SkillHUDPresentationController.h"
#include "../SkillHUD/AttunementSlot.h"
#include "../SkillHUD/PotionAttunement.h"
#include "../SkillHUD/SkillSlot.h"
#include "../SkillHUD/PotionSlot.h"

namespace AttunementSkillbar {

    /**
     * The record kind written to the save game.
     */
    constexpr std::uint32_t PluginID = 'BMSK';

    /**
     * The record kind written to the save game.
     */
    constexpr std::uint32_t RecordKind = 'BMCF';

    /**
     * The record kind written to the save game for cooldown data.
     */
    constexpr std::uint32_t CooldownRecordKind = 'BMRC';

    /**
     * The record kind written to the save game for local form data.
     */
    constexpr std::uint32_t LocalFormRecordKind = 'BMLF';

    /**
     * Describes data that is persisted with a save game.
     */
    struct PersistenceData {

        /**
         * The behaviour configuration.
         */
        SpellCastControllerConfiguration behaviourConfiguration {};

        /**
         * The input configuration.
         */
        InputEventControllerConfiguration inputConfiguration {};

        /**
         * The UI configuration.
         */
        SkillHUDConfiguration HUDConfiguration {};

        /**
         * The currently active potion.
         */
        uint32_t activePotion { 0 };

        /**
         * The show/hide configuration.
         */
        SkillHUDPresentationConfiguration showHideConfiguration {};

    };

    /**
     * A class that manages reading and writing configuration to the save game file
     * and resetting configuration when the game unloads
     */
    class PersistenceController {
        private:

            /**
             * Invoked to load the configuration from the game file.
             * @param coder         The serialization interface to use for reading.
             */
            static void LoadConfiguration(SKSE::SerializationInterface *coder);

            /**
             * Invoked to load the tracked cooldowns from the game file.
             * @param coder         The serialization interface to use for reading.
             */
            static void LoadCooldowns(SKSE::SerializationInterface *coder);

            /**
             * Invoked to load the local form configurations from the game file.
             * @param coder             The serialization interface to use for reading.
             * @param skillConfigs      An array that will be initialized to the read skill configurations.
             * @param potionsConfigs    An array that will be initialized to the read potion configurations.
             */
            static void LoadLocalFormConfigurations(
                SKSE::SerializationInterface *coder,
                SkillSlotLocalFormConfiguration **skillConfigs,
                SkillSlotLocalFormConfiguration **potionConfigs
            );

            /**
             * Writes the specified local form configuration to the specified coder.
             * @param coder         The serialization interface to use for writing.
             * @param config        The configuration to store.
             */
            static void WriteLocalFormConfiguration(SKSE::SerializationInterface *coder, SkillSlotLocalFormConfiguration config);


            /**
             * Reads a local form configuration from the specified coder.
             * @param coder         The serialization interface to use for reading.
             */
            static SkillSlotLocalFormConfiguration ReadLocalFormConfiguration(SKSE::SerializationInterface *coder);

        public:

            /**
             * Installs the persistence controller to handle saving and loading games.
             */
            static void Install();

            /**
             * Invoked to save the configuration with the game file.
             * @param coder         The serialization interface to use for writing.
             */
            static void Save(SKSE::SerializationInterface *coder);

            /**
             * Invoked to load the configuration from the game file.
             * @param coder         The serialization interface to use for reading.
             */
            static void Load(SKSE::SerializationInterface *coder);

            /**
             * Invoked to reset state prior to exiting or loading a new game file.
             */
            static void Revert(SKSE::SerializationInterface *);
    };
}