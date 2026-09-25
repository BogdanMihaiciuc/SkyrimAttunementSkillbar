#include "../SkillHUD/SkillHUD.h"
#include "../Input/SpellCastController.h"

namespace AttunementSkillbar {

    /**
     * A class that manages reading and writing configuration to the skill HUD
     * by providing a various Papyrus function that the MCM configuration script uses.
     */
    class ConfigurationController {

        public:

            /**
             * Initializes the configuration controller's script interface.
             */
            static bool InitializeScript(RE::BSScript::IVirtualMachine *vm);

            /**
             * Returns the anchor kind for the specified integer.
             * @param value     The integer value.
             * @returns         The corrseponding anchor kind.
             */
            static SkillHUDConfigurationAnchor AnchorForInt(int32_t value);
            
            /**
             * Returns the skill cast behaviour for the specified integer.
             * @param value     The integer value.
             * @returns         The corresponding skill cast behaviour.
             */
            static SkillCastBehaviour SkillCastBehaviourForInt(int32_t value);

            /**
             * Returns the potion select behaviour for the specified integer.
             * @param value     The integer value.
             * @returns         The corresponding potion select behaviour.
             */
            static PotionSelectBehaviour PotionSelectBehaviourForInt(int32_t value);



            //MARK: Functional configuration
            
            /**
             * Gets the number of configured attunements.
             * @returns         The number of configured attunements.
             */
            static int32_t GetAttunementCount(RE::StaticFunctionTag *);

            /**
             * Sets the number of configured attunements.
             * @param count         The number of attunements to use.
             */
            static void SetAttunementCount(RE::StaticFunctionTag *, int32_t count);
            
            /**
             * Gets the number of configured skills per attunement.
             * @returns         The number of configured skills.
             */
            static int32_t GetSkillCount(RE::StaticFunctionTag *);

            /**
             * Sets the number of configured skills per attunement.
             * @param count         The number of skills to use.
             */
            static void SetSkillCount(RE::StaticFunctionTag *, int32_t count);

            /**
             * Gets the texture ID to use for the attunement at the specified index.
             * @param index         The index of the attunement whose texture ID should
             *                      be obtained.
             * @returns             The texture ID.
             */
            static int32_t GetTextureIDForAttunement(RE::StaticFunctionTag *, int32_t index);

            /**
             * Sets the texture ID to use for the attunement at the specified index.
             * @param index         The index of the attunement whose texture ID should be set.
             * @param textureID     The new texture ID to use.
             */
            static void SetTextureIDForAttunement(RE::StaticFunctionTag *, int32_t index, int32_t textureID);

            /**
             * Gets the current skill cast behaviour.
             * @returns         The current skill cast behaviour.
             */
            static int32_t GetSkillCastBehaviour(RE::StaticFunctionTag *);

            /**
             * Sets the skill cast behaviour.
             * @param behaviour     The skill cast behaviour to set.
             */
            static void SetSkillCastBehaviour(RE::StaticFunctionTag *, int32_t behaviour);

            /**
             * Gets the current potion select behaviour.
             * @returns             The current potion select behaviour.
             */
            static int32_t GetPotionSelectBehaviour(RE::StaticFunctionTag *);

            /**
             * Sets the potion select behaviour.
             * @param behaviour     The potion select behaviour to set.
             */
            static void SetPotionSelectBehaviour(RE::StaticFunctionTag *, int32_t behaviour);

            /**
             * Gets whether the equipped spell should be auto-cast.
             * @returns             True if auto-casting of the equipped spell is enabled,
             *                      otherwise false.
             */
            static bool GetAutoCastsEquippedSpell(RE::StaticFunctionTag *);

            /**
             * Sets whether the equipped spell should be auto-cast.
             * @param enabled       True to enable auto-casting, false to disable.
             */
            static void SetAutoCastsEquippedSpell(RE::StaticFunctionTag *, bool enabled);

            /**
             * Gets whether the dual cast preference for equipped spells is enabled.
             * @returns     True if dual cast is preferred when the spell is equipped in both 
             *              hands, otherwise false.
             */
            static bool GetPrefersDualCast(RE::StaticFunctionTag *);

            /**
             * Sets the dual cast preference for equipped spells.
             * @param enabled           True to prefer dual casting, false to disable.
             */
            static void SetPrefersDualCast(RE::StaticFunctionTag *, bool enabled);

            //MARK: Keybinds

            /**
             * Returns the currently assigned keybind for the attunement at the specified index.
             * @param index         The index of the attunement whose keybind should be obtained.
             * @param keyID         The ID of the key to use.
             */
            static int32_t GetKeybindForAttunement(RE::StaticFunctionTag *, int32_t index);

            /**
             * Assigns the specified keybind for the attunement at the specified index.
             * @param index         The index of the attunement whose keybind should be obtained.
             * @param keyID         The ID of the key to use.
             */
            static void SetKeybindForAttunement(RE::StaticFunctionTag *, int32_t index, int32_t keyID);

            /**
             * Gets the modifier key ID used to bring up the skill assignment menu.
             * @returns         The assignment modifier key ID.
             */
            static uint32_t GetAssignmentModifierKeyID(RE::StaticFunctionTag *);

            /**
             * Sets the modifier key ID used to bring up the skill assignment menu.
             * @param keyID     The new assignment modifier key ID.
             */
            static void SetAssignmentModifierKeyID(RE::StaticFunctionTag *, uint32_t keyID);


            /**
             * Gets the key ID used to assign a skill to the right hand.
             * @returns         The right-hand assignment key ID.
             */
            static uint32_t GetAssignRightHandKeyID(RE::StaticFunctionTag *);

            /**
             * Sets the key ID used to assign a skill to the right hand.
             * @param keyID     The new right-hand assignment key ID.
             */
            static void SetAssignRightHandKeyID(RE::StaticFunctionTag *, uint32_t keyID);
            

            /**
             * Gets the key ID used to assign a skill to the left hand.
             * @returns         The left-hand assignment key ID.
             */
            static uint32_t GetAssignLeftHandKeyID(RE::StaticFunctionTag *);

            /**
             * Sets the key ID used to assign a skill to the left hand.
             * @param keyID     The new left-hand assignment key ID.
             */
            static void SetAssignLeftHandKeyID(RE::StaticFunctionTag *, uint32_t keyID);
            

            /**
             * Gets the modifier key ID used to modify an assignment to prefer dual casting.
             * @returns         The dual-cast modifier key ID.
             */
            static uint32_t GetAssignDualCastModifierKeyID(RE::StaticFunctionTag *);

            /**
             * Sets the modifier key ID used to modify an assignment to prefer dual casting.
             * @param keyID     The new dual-cast modifier key ID.
             */
            static void SetAssignDualCastModifierKeyID(RE::StaticFunctionTag *, uint32_t keyID);
            

            /**
             * Gets the key used to activate or select potions
             * @returns         The potion key ID.
             */
            static uint32_t GetPotionKeyID(RE::StaticFunctionTag *);

            /**
             * Sets the key used to activate or select potions
             * @param keyID     The new potion key ID.
             */
            static void SetPotionKeyID(RE::StaticFunctionTag *, uint32_t keyID);
            

            /**
             * Gets the key ID used to select the next attunement.
             * @returns         The next attunement key ID.
             */
            static uint32_t GetNextAttunementKeyID(RE::StaticFunctionTag *);

            /**
             * Sets the key ID used to select the next attunement.
             * @param keyID     The new next attunement key ID.
             */
            static void SetNextAttunementKeyID(RE::StaticFunctionTag *, int32_t keyID);


            /**
             * Gets the key ID used to select the previous attunement.
             * @returns         The previous attunement key ID.
             */
            static uint32_t GetPreviousAttunementKeyID(RE::StaticFunctionTag *);

            /**
             * Sets the key ID used to select the previous attunement.
             * @param keyID     The new previous attunement key ID.
             */
            static void SetPreviousAttunementKeyID(RE::StaticFunctionTag *, int32_t keyID);


            /**
             * Returns the currently assigned keybind for the skill at the specified index.
             * @param index         The index of the skill whose keybind should be obtained.
             * @returns             The current keybind.
             */
            static int32_t GetKeybindForSkill(RE::StaticFunctionTag *, int32_t index);

            /**
             * Assigns the specified keybind for the skill at the specified index.
             * @param index         The index of the skill whose keybind should be obtained.
             * @returns             The current keybind.
             */
            static void SetKeybindForSkill(RE::StaticFunctionTag *, int32_t index, int32_t keyID);

            // MARK: Show/hide configuration
            
            /**
             * Gets whether the skill HUD should always be shown.
             * @returns             True if the skill HUD is always shown, false otherwise.
             */
            static bool GetShowsAlways(RE::StaticFunctionTag *);

            /**
             * Sets whether the skill HUD should always be shown.
             * @param enabled       True if the skill HUD is always shown, false otherwise.
             */
            static void SetShowsAlways(RE::StaticFunctionTag *, bool enabled);


            /**
             * Gets whether the skill HUD should be shown while in combat.
             * @returns             True if the skill HUD is shown in combat, false otherwise.
             */
            static bool GetShowsInCombat(RE::StaticFunctionTag *);

            /**
             * Sets whether the skill HUD should be shown while in combat.
             * @param enabled       True if the skill HUD is shown in combat, false otherwise.
             */
            static void SetShowsInCombat(RE::StaticFunctionTag *, bool enabled);


            /**
             * Gets whether the skill HUD should be shown while the player's weapons are drawn.
             * @returns             True if the skill HUD is shown the player's weapons are drawn,
             *                      false otherwise.
             */
            static bool GetShowsWeaponsDrawn(RE::StaticFunctionTag *);

            /**
             * Sets whether the skill HUD should be shown while the player's weapons are drawn.
             * @param enabled       True if the skill HUD is shown the player's weapons are drawn,
             *                      false otherwise.
             */
            static void SetShowsWeaponsDrawn(RE::StaticFunctionTag *, bool enabled);

            // MARK: Layout configuration

            /**
             * Gets the currently used HUD scale.
             * @returns     The current HUD scale.
             */
            static float GetHUDScale(RE::StaticFunctionTag *);

            /**
             * Sets the scale that the HUD should use.
             * @param scale     The scale that the HUD should use.
             */
            static void SetHUDScale(RE::StaticFunctionTag *, float scale);

            /**
             * Gets the horizontal anchor kind.
             * @returns         The currently configured horizontal anchor.
             */
            static int32_t GetHorizontalAnchorKind(RE::StaticFunctionTag *);
            
            /**
             * Sets the horizontal anchor kind that the HUD will use.
             * @param kind      The new anchor kind to use.
             */
            static void SetHorizontalAnchorKind(RE::StaticFunctionTag *, int32_t kind);
            
            /**
             * Gets the vertical anchor kind.
             * @returns         The currently configured vertical anchor.
             */
            static int32_t GetVerticalAnchorKind(RE::StaticFunctionTag *);

            /**
             * Sets the vertical anchor kind that the HUD will use.
             * @param kind      The new anchor kind to use.
             */
            static void SetVerticalAnchorKind(RE::StaticFunctionTag *, int32_t kind);
            
            // -----------

            /**
             * Gets the horizontal anchor point.
             * @returns         The currently configured horizontal anchor point.
             */
            static float GetHorizontalAnchorPoint(RE::StaticFunctionTag *);

            /**
             * Sets the horizontal anchor point that the HUD will use.
             * @param point     The new anchor point to use.
             */
            static void SetHorizontalAnchorPoint(RE::StaticFunctionTag *, float point);


            /**
             * Gets the vertical anchor point.
             * @returns         The currently configured vertical anchor point.
             */
            static float GetVerticalAnchorPoint(RE::StaticFunctionTag *);

            /**
             * Sets the vertical anchor point that the HUD will use.
             * @param point     The new anchor point to use.
             */
            static void SetVerticalAnchorPoint(RE::StaticFunctionTag *, float point);
            

            /**
             * Gets the currently used HUD left margin.
             * @returns         The current HUD left margin.
             */
            static float GetMarginLeft(RE::StaticFunctionTag *);

            /**
             * Sets the left margin that the HUD should use.
             * @param margin    The left margin that the HUD should use.
             */
            static void SetMarginLeft(RE::StaticFunctionTag *, float margin);

            /**
             * Gets the currently used HUD right margin.
             * @returns         The current HUD right margin.
             */
            static float GetMarginRight(RE::StaticFunctionTag *);

            /**
             * Sets the right margin that the HUD should use.
             * @param margin    The right margin that the HUD should use.
             */
            static void SetMarginRight(RE::StaticFunctionTag *, float margin);

    };

}