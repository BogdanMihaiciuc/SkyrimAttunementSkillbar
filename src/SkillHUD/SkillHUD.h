#pragma once
#include "imgui_internal.h"

namespace AttunementSkillbar {

    class PotionSlot;
    class PotionSlotIndicator;
    class AttunementSlot;
    class PotionAttunement;
    struct AttunementSlotConfiguration;
    struct SkillSlotConfiguration;
    struct SkillSlotSkillConfiguration;
    struct PotionSlotConfiguration;

    /**
     * The offset between the attunement texture indices and the associated
     * actor values.
     */
    const uint32_t AttunementTextureOffset = 18;

    struct SkillHUDTheme {

        /**
         * The size of a skill slot before any scaling takes place.
         */
        float SkillSlotSizeBase { 64.0f };

        /**
         * The amount of spacing to keep between skill slots.
         */
        float SkillSlotSpacingBase { 0.0f };

        /**
         * The size of a skill slot keybind indicator.
         */
        float SkillSlotKeybindHeightBase { 24.0f };

        /**
         * The amount of scaling to apply to a skill slot whose
         * associated keybind is pressed.
         */
        float SkillSlotPressedScale { 0.9f };

        /**
         * The size of the indicator showing which hand a skill is casted with.
         */
        float SkillSlotHandIndicatorSizeBase { 16.0f };

        /**
         * The size of an attunement slot that is not selected before
         * any scaling takes place.
         */
        float AttunementSlotSizeDeselected { 36.0f };

        /**
         * The amount of spacing to keep between attunement slots.
         */
        float AttunementSlotSpacingBase { 0.0f };

        /**
         * The size of an attunement slot keybind indicator.
         */
        float AttunementSlotKeybindHeightBase { 16.0f };

        /**
         * The amount of scaling to apply to a skill slot whose
         * associated keybind is pressed.
         */
        float AttunementSlotPressedScale { 0.8f };

        /**
         * The amount of scaling to apply to the attunement slot that
         * is selected.
         */
        float AttunementSlotSelectedScale { 1.5f };

        /**
         * The distance between the bottom edge of the attunements and
         * the top edge of the skill slots.
         */
        float SkillAttunementGap { 16.0f };

        /**
         * The distance between the edges of the cycle keybind keys and the attunements.
         */
        float AttunementCycleKeybindGap { 16.0f };

        /**
         * The distance between the right edge of the potion key and
         * the left edge of the attunement and skills.
         */
        float PotionSkillGap { 32.0f };

        /**
         * The amount of time in seconds it takes for the skill slot press animation to play.
         */
        float SkillSlotPressedAnimationDuration { 0.2f };

        /**
         * The amount of time in seconds it takes for the attunement change animation to play.
         */
        float AttunementChangeAnimationDuration { 0.3f };

    };

    /**
     * The maximum permitted number of attunement slots.
     */
    const uint32_t MaxAttunementSlots = 20;

    /**
     * The maximum permitted skill slots.
     */
    const uint32_t MaxSkillSlots = 30;

    /**
     * Interpolates the specified normalized time using an ease in quart easing function.
     * @param time      The normalized time.
     * @returns         The interpolated time.
     */
    float EaseInQuartWithTime(float time);

    /**
     * Interpolates the specified normalized time using an ease out quart easing function.
     * @param time      The normalized time.
     * @returns         The interpolated time.
     */
    float EaseOutQuartWithTime(float time);

    /**
     * Interpolates the specified normalized time using an ease in out quart easing function.
     * @param time      The normalized time.
     * @returns         The interpolated time.
     */
    float EaseInOutQuartWithTime(float time);

    /**
     * Returns the normalized animation time for the specified current time
     * and max time.
     * @param currentTime       The current animation time in seconds.
     * @param maxTime           The maximum animation time.
     */
    float NormalizedTime(float currentTime, float maxTime);

    /**
     * Returns an interpolated value between the specified start and end values
     * based on the normalized time.
     * @param start     The starting value.
     * @param end       The target value.
     * @param time      The normalized time.
     */
    float InterpolatedValue(float start, float end, float time);

    /**
     * Describes a texture references that can be used to draw the
     * icon for a skill.
     */
    struct SkillTexture {

        /**
         * The ID of the texture atlas containing the skill icon.
         */
        void *textureID { nullptr };

        /**
         * The top left corner of the skill icon as a percent relative to the atlas.
         */
        ImVec2 topLeft;

        /**
         * The tbottom right corner of the skill icon as a percent relative to the atlas.
         */
        ImVec2 bottomRight;

        bool operator==(const SkillTexture &other) const {
            return
                textureID == other.textureID &&
                topLeft.x == other.topLeft.x &&
                topLeft.y == other.topLeft.y &&
                bottomRight.x == other.bottomRight.x &&
                bottomRight.y == other.bottomRight.y;
        }
    };


    /**
     * An extension of skill textures that adds a size ratio and is used for describing
     * keybind textures.
     */
    struct KeybindTexture : public SkillTexture {

        /**
         * The base ratio between the width and height of the texture.
         */
        float sizeRatio;

    };

    /**
     * Constants which describe the size of the screen along an axis
     * to which the skill HUD is anchored.
     */
    enum SkillHUDConfigurationAnchor {

        /**
         * Indicates that the skill HUD is anchored to the left or top of the screen.
         */
        Start = 1,

        /**
         * Indicates that the skill HUD is anchored to the center x or y of the screen.
         */
        Center = 2,

        /**
         * Indicates that the skill HUD is anchored to the right or bottom of the screen.
         */
        End = 3,
    };

    /**
     * An object that contains the visual layout configuration for the UI portion of
     * the attunement skillbar.
     */
    struct SkillHUDLayoutConfiguration {

        /**
         * The size scale to apply to the entire HUD as requested by the user.
         * The final scale may be smaller to fit more skill slots.
         */
        float sizeScale { 1.0f };

        /**
         * The amount of margin to keep, in 1080p-relative pixels, at a minimum to the
         * left side of the screen. When the HUD would increase past this margin, it
         * will be scaled instead.
         */
        float marginLeft { 16.0f };

        /**
         * The amount of margin to keep, in 1080p-relative pixels, at a minimum to the
         * right side of the screen. When the HUD would increase past this margin, it
         * will be scaled instead.
         */
        float marginRight { 16.0f };

        /**
         * The anchor point relative to each axis anchor.
         */
        ImVec2 anchorPoint { 0.0f, 96.0f };

        /**
         * The horizontal anchor.
         */
        SkillHUDConfigurationAnchor horizontalAnchor { Center };

        /**
         * The vertical anchor.
         */
        SkillHUDConfigurationAnchor verticalAnchor { End };

        /**
         * Controls whether the potion slot is shown.
         */
        bool showsPotion { true };

    };

    /**
     * An object that contains the configuration for the UI portion of
     * the attunement skillbar.
     */
    struct SkillHUDConfiguration : public SkillHUDLayoutConfiguration {

        /**
         * The number of available attunements.
         */
        uint32_t attunementCount { 4 };

        /**
         * The number of skill slots for each attunement.
         */
        uint32_t skillSlotCount { 10 };

    };

    /**
     * A struct that describes the total and remaining cooldown of a skill.
     */
    struct SkillCooldown {

        /**
         * The game day when cooldown started.
         */
        float start { 0.0f };

        /**
         * The remaining cooldown percent.
         */
        float remaining { 0.0f };

    };

    /**
     * Represents a attunement skillbar UI containing both the attunement
     * bar and the skillbars associated with each attunement.
     */
    class SkillHUD :
        public RE::BSTEventSink<RE::TESContainerChangedEvent>,
        public RE::BSTEventSink<RE::TESSpellCastEvent>
    {
        friend class AttunementSlot;
        friend class SkillSlot;
        friend class SkillAssignmentHUD;
        friend class SkillHUDPresentationController;
        friend class PersistenceController;
        private:

            /**
             * Set to true after a configuration is applied to this HUD.
             */
            bool _hasConfiguration { false };

            /**
             * The shared HUD instance.
             */
            static SkillHUD *_sharedHUD;

            /**
             * The theme constants.
             */
            static SkillHUDTheme _theme;

            /**
             * A map that holds the associated skill textures for all skills.
             */
            static std::unordered_map<RE::FormID, SkillTexture> _skillTextureMap;

            /**
             * A map that holds the associated skill textures for generic potions.
             */
            static std::unordered_map<RE::FormID, SkillTexture> _genericPotionTextureMap;

            /**
             * A map that holds the internal non-skill textures.
             */
            static std::unordered_map<RE::FormID, SkillTexture> _internalTextureMap;

            /**
             * A map that holds the keybind textures.
             */
            static std::unordered_map<RE::FormID, KeybindTexture> _keybindTextureMap;

            /**
             * The current configuration used by the skill HUD.
             */
            SkillHUDConfiguration _configuration {};

            /**
             * The current opacity of the skill HUD.
             */
            float _opacity { 1.0f };

            /**
             * The amount of Y translation to apply to all elements of the HUD.
             */
            float _translateY { 0.0f };

            /**
             * The computed position of the skill HUD.
             */
            ImVec2 _position { 0.0f, 0.0f };

            /**
             * The top-left position in pixels at which to start drawing the skill slots.
             */
            ImVec2 _skillPosition { 0.0f, 0.0f };

            /**
             * The top-left position in pixels at which to start drawing the potion slot.
             */
            ImVec2 _potionPosition { 0.0f, 0.0f };

            /**
             * The computed scale of the skill HUD.
             */
            float _scale { 1.0f };

            /**
             * The attunement slots that are available for this skillbar.
             */
            AttunementSlot **_attunementSlots { nullptr };

            /**
             * The single potion attunement slot.
             */
            PotionAttunement *_potionAttunement { nullptr };

            /**
             * The skill slot that indicates the currently selected potion.
             */
            PotionSlotIndicator *_potionIndicator { nullptr };

            /**
             * The index of the currently selected potion.
             */
            uint32_t _activePotionSlot { 0 };

            /**
             * The currently active attunement index.
             */
            uint32_t _activeAttunement { 0 };

            /**
             * The previously active attunement index.
             */
            uint32_t _previouslyActiveAttunement { 0 };

            /**
             * The time delta when attunement change took place.
             */
            float _attunementChangeTime { -1.0f };

            /**
             * Set to true if this skill hud is showing the potions attunement
             * in place of the regular skill attunements.
             */
            bool _showsPotions { false };

            /**
             * Set to true while the animation swapping between skills and potions
             * is running for this skill hud.
             */
            bool _swappingPotions { false };

            /**
             * The time delta since the potion swapping animation started.
             */
            float _potionChangeTime { -1.0f };

            /**
             * A map that is used to track power cooldowns.
             */
            std::unordered_map<RE::FormID, SkillCooldown> _powerCooldownMap;

            /**
             * The current shout total cooldown.
             */
            float _shoutCooldownTotalTime { 0.0f };

            /**
             * A set used for quick access to shout cooldowns.
             */
            std::set<RE::FormID> _shoutSet;

            /**
             * A mutex used to guard concurrent access to the cooldown map
             */
            std::mutex _powerCooldownLock;

            /**
             * Updates the power cooldowns.
             */
            void AdvanceCooldowns();

            /**
             * Initializes the cooldown data with the specified shout and power cooldowns.
             * @param shoutCooldown         The current total shout cooldown.
             * @param powerCooldowns        The tracked power cooldowns.
             */
            void SetCooldownData(float shoutCooldown, std::unordered_map<RE::FormID, SkillCooldown> powerCooldowns);

            /**
             * The keybind texture for the next attunement keybind.
             */
            KeybindTexture _nextAttunementKeybind {};

            /**
             * The keybind texture for the previous attunement keybind.
             */
            KeybindTexture _previousAttunementKeybind {};

            /**
             * Creates the attunements and skill slots from the current
             * configuration.
             * @param configurations        The configurations of the attunement slots, or nullptr to
             *                              use the default configuration.
             * @param skillConfigurations   The configurations of the skill slots, or nullptr to
             *                              use the default configuration.
             */
            void CreateAttunements(
                AttunementSlotConfiguration *configurations = nullptr,
                SkillSlotConfiguration *skillConfigurations = nullptr
            );

            /**
             * Removes the current attunement instances and frees their memory.
             */
            void ReleaseAttunements();

            /**
             * Prepares the layout for each attunement and skill slot based on the current configuration
             * and screen characteristics.
             */
            void PrepareLayout();

            /**
             * Renders the attunement cycle keybinds if they are defined.
             * @param delta             The time delta since the last frame was drawn.
             * @param attunementOffset  The offset to apply to deselected attunements.
             * @param scaleY            An optional Y scale to apply to the keybinds.
             */
            void RenderAttunementCycleKeybinds(float delta, float attunementOffset, float scaleY = 1.0f);

            /**
             * Renders the skill and attunements HUD.
             * @param delta             The time delta since the last frame was drawn.
             * @param attunementOffset  The offset to apply to deselected attunements.
             */
            void RenderAttunements(float delta, float attunementOffset);

            /**
             * Renders the potions HUD.
             * @param delta         The time delta since the last frame was drawn.
             * @param attunementOffset  The offset to apply to deselected attunements.
             */
            void RenderPotions(float delta, float attunementOffset);

            /**
             * Advances the animations for the skill slots of the specified attunement slot
             * without performing any actual rendering.
             * @param slot      The attunement slot whose skill slot animations should advance.
             * @param delta     The delta time since the previously drawn frame.
             * @param scaleY    The flipping animation scale Y value.
             */
            void RenderAttunementSlotSkills(AttunementSlot *slot, float delta, float scaleY);

            /**
             * Advances the animations for the skill slots of the specified attunement slot
             * without performing any actual rendering.
             * @param slot      The attunement slot whose skill slot animations should advance.
             * @param delta     The delta time since the previously drawn frame.
             */
            void AdvanceAttunementSlotAnimations(AttunementSlot *slot, float delta);

            /**
             * Loads the skill icon textures from the file at the specified path.
             * @param path      The path from which to load the textures.
             */
            static void LoadIconTexturesFromFile(const std::filesystem::path &path);

            /**
             * Loads the keybind icon textures from the file at the specified path.
             * @param path      The path from which to load the textures.
             */
            static void LoadKeybindTexturesFromFile(const std::filesystem::path &path);

            /**
             * Loads the theme values from the file at the specified path.
             * @param path      The path from which to load the theme.
             */
            static void LoadThemeFromFile(const std::filesystem::path &path);

        public:

            /**
             * The current theme.
             */
            static inline const SkillHUDTheme &Theme() {
                return _theme;
            }

            /**
             * The texture used to draw the button border.
             */
            static SkillTexture buttonTexture;

            /**
             * Texture used for buttons that have no other appropriate texture.
             */
            static SkillTexture emptyButtonTexture;

            /**
             * Texture used for the left hand casting source indicator.
             */
            static SkillTexture leftHandTexture;

            /**
             * Texture used for the left hand casting source indicator.
             */
            static SkillTexture rightHandTexture;

            /**
             * Returns the current skill configuration of the skill slot at the specified
             * index in the currently active attunement.
             * @param index     The skill index.
             */
            SkillSlotSkillConfiguration GetSkillConfigurationAtIndex(uint32_t index);

            /**
             * Initializes the specified skill slot and potion slot configurations using
             * the configuration of the potion at the specified index.
             * @param index         The potion index.
             * @param skill         The skill slot configuration to initialize.
             * @param potion        The potion slot configuration to initialize.
             */
            void InitializePotionConfigurationWithIndex(uint32_t index, SkillSlotSkillConfiguration &skill, PotionSlotConfiguration &potion);

            /**
             * Plays the pressing animation on the skill at the specified index.
             * @param index     The skill index.
             * @param potion    When set to true, the skill will be selected from the
             *                  potion attunement instead of the active attunement.
             */
            void PressSkillAtIndex(uint32_t index, bool potion = false);

            /**
             * Plays the de-pressing animation on the skill at the specified index.
             * @param index     The skill index.
             * @param potion    When set to true, the skill will be selected from the
             *                  potion attunement instead of the active attunement.
             */
            void ReleaseSkillAtIndex(uint32_t index, bool potion = false);

            /**
             * Plays the pressing animation on the potion indicator.
             */
            void PressPotionIndicator();

            /**
             * Plays the de-pressing animation on the potion indciator.
             */
            void ReleasePotionIndicator();

            /**
             * Controls whether this skill HUD shows skills and attunements or potions.
             * @param shows     When set to true, potions are shown, otherwise skills are shown.
             * @param animated  When set to true, if this causes the currently displayed slots to
             *                  change, this change will be animated, otherwise it will be instant.
             */
            void SetShowsPotions(bool shows, bool animated);

            /**
             * Gets the potion slot index displayed by the potion indicator.
             * @returns             The index of the potion displayed in the indicator.
             */
            uint32_t GetActivePotion();

            /**
             * Updates the potion slot displayed by the potion indicator.
             * @param index         The index of the potion to display.
             */
            void SetActivePotion(uint32_t index);

            /**
             * Obtains the texture to use for an attunement, falling back
             * to a default texture if the requested one could not be found.
             * @param ID            The ID of the texture to use.
             * @returns             A struct describing the texture to use.
             */
            static SkillTexture GetTextureForAttunement(uint32_t ID);

            /**
             * Obtains the texture to use for a generic potion, falling back
             * to a default texture if the requested one could not be found.
             * @param ID            The ID of the effect of the potion.
             * @returns             A struct describing the texture to use.
             */
            static SkillTexture GetTextureForGenericPotion(uint32_t ID);

            /**
             * Obtains the texture to use for the specified spell, falling back
             * to a default texture if a specific one could not be found.
             * @param ID            The TESForm ID of the skill.
             * @returns             A struct describing the texture to use.
             */
            static SkillTexture GetTextureForSkill(uint32_t ID);

            /**
             * Obtains the texture to use for a skill or attunement slot that doesn't
             * have a skill or static texture assigned to it.
             * @param filename      The filename where the spell was defined.
             * @param localID       The package local ID of the spell.
             * @returns             A struct describing the texture to use.
             */
            static SkillTexture GetEmptySkillSlotTexture();

            /**
             * Obtains the texture to use for the border of a skill or attunement slot.
             * @returns             A struct describing the texture to use.
             */
            static SkillTexture GetSkillSlotBorderTexture();

            /**
             * Obtains the texture to use for the specified casting source indicator.
             * @returns             A struct describing the texture to use, if any is available.
             */
            static SkillTexture GetCastingSourceIndicatorTexture(RE::MagicSystem::CastingSource source);

            /**
             * Loads the skill icon configuration from disk by looking up all json
             * files in the "Data\SkillIcons" folder and loading the textures they
             * define.
             */
            static void Initialize();

            /**
             * Obtains a reference to the shared HUD. A configuration must
             * be set on it before it can be used.
             */
            static SkillHUD *SharedHUD() {
                if (!_sharedHUD) {
                    _sharedHUD = new SkillHUD();
                }

                return _sharedHUD;
            }

            /**
             * Causes this skill HUD to use the default configuration.
             */
            void UseDefaultConfiguration();

            /**
             * Updates the configuration of this skill HUD.
             */
            void SetConfiguration(
                SkillHUDConfiguration configuration,
                AttunementSlotConfiguration *attunementConfigurations,
                SkillSlotConfiguration *slotConfigurations
            );

            /**
             * Renders the skill HUD.
             */
            void Render();

            /**
             * Advances the animations of the skill HUD components without rendering.
             */
            void AdvanceAnimations();

            /**
             * Activates the attunement at the specified index. This method
             * has no effect if the attunement at the specified index is already
             * activated or does not exist.
             * @param index     The index of the attunement to activate.
             * @param animated  Whether this change will be animated.
             */
            void ActivateAttunement(uint32_t index, bool animated = true);

            /**
             * Activates the next attunement, or the first attunement if the currently
             * active attunement is the last.
             * @param animated  Whether this change will be animated.
             */
            void ActivateNextAttunement(bool animated = true);

            /**
             * Activates the previous attunement, or the last attunement if the currently
             * active attunement is the first.
             * @param animated  Whether this change will be animated.
             */
            void ActivatePreviousAttunement(bool animated = true);

            /**
             * Assigns the specified keybind texture for the specified keybind.
             * @param keyID         The ID of the key for the keybind.
             * @param index         The index of the attunement slot. 
             */
            void AssignAttunementKeybind(uint32_t keybind, uint32_t index);

            /**
             * Assigns the specified skill to the specified slot index in the currently
             * active attunement.
             * @param skill         The skill to assign.
             * @param index         The index of the skill to assign the skill to.
             */
            void AssignSkill(SkillSlotSkillConfiguration skill, uint32_t index);

            /**
             * Assigns the specified keybind texture for all skills at the specified index.
             * @param keyID         The ID of the key for the keybind.
             * @param index         The index of the skill slot. 
             */
            void AssignSkillKeybind(uint32_t keybind, uint32_t index);

            /**
             * Assigns the specified keybind texture for the potion indicator.
             * @param keyID         The ID of the key for the keybind.
             */
            void AssignPotionKeybind(uint32_t keybind);

            /**
             * Assigns the specified keybind texture for the next attunement indicator.
             * @param keyID         The ID of the key for the keybind.
             */
            void AssignNextAttunementKeybind(uint32_t keybind);

            /**
             * Assigns the specified keybind texture for the previous attunement indicator.
             * @param keyID         The ID of the key for the keybind.
             */
            void AssignPreviousAttunementKeybind(uint32_t keybind);

            /**
             * Assigns the specified skill to the specified slot index in the specified
             * attunement slot.
             * @param skill         The skill to assign.
             * @param attunement    The attunement index.
             * @param index         The index of the skill to assign the skill to.
             */
            void AssignSkillForAttunement(SkillSlotSkillConfiguration skill, uint32_t attunementIndex, uint32_t index);

            /**
             * Assigns the specified potion to the specified slot index in the potion attunement.
             * @param skill         The potion skill to assign.
             * @param potion        Potion kind configuration.
             * @param index         The index of the skill to assign the skill to.
             */
            void AssignPotion(SkillSlotSkillConfiguration skill, PotionSlotConfiguration potion, uint32_t index);

            /**
             * Counts and returns the number of potions the player has matching the specified configuration.
             * @param skill         The base potion form.
             * @param potion        The generic potion.
             * @returns             The number of matching potions in the player's inventory.
             */
            uint32_t CountPotion(SkillSlotSkillConfiguration skill, PotionSlotConfiguration potion);

            /**
             * Updates the displayed count of the specified scroll if it is assigned to any potion slot.
             * @param scrollID      The ID of the scroll whose count to update.
             */
            void UpdateScrollCount(RE::FormID scrollID);

            RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* event, RE::BSTEventSource<RE::TESContainerChangedEvent>*) override;

            /**
             * Obtains the remaining cooldown, as a percent, of the specified skill.
             */
            float CooldownForSkill(RE::FormID skill);

            RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* event, RE::BSTEventSource<RE::TESSpellCastEvent>*) override;

            // MARK: Configuration

            /**
             * Resets the state of this skill HUD in preparation for the
             * current session ending.
             */
            void ResetState();

            /**
             * Gets the currently configured attunement count.
             */
            uint32_t GetAttunementCount();

            /**
             * Configures the skill HUD to use the specified number of attunements. If this
             * is less than the current number of attunements, the extra attunements will be
             * deleted with their skill slots and configurations.
             * @param count         The new attunement count to use.
             */
            void SetAttunementCount(uint32_t count);

            /**
             * Gets the currently configured skill slot count.
             */
            uint32_t GetSkillCount();

            /**
             * Configures the skill HUD to use the specified number of skill slots. If this
             * is less than the current number of skill slots, the extra skill slots will be
             * deleted with their skill slots and configurations.
             * @param count         The new attunement count to use.
             */
            void SetSkillCount(uint32_t count);

            /**
             * Returns the texture ID that is configured for the attunement at the specified index.
             * @param index         The index of the attunement.
             */
            uint32_t GetTextureIDForAttunement(uint32_t index);

            /**
             * Updates the attunement at the specified index with the new texture.
             * @param index         The index of the attunement.
             * @param textureID     The new texture ID to use.
             */
            void SetTextureIDForAttunement(uint32_t index, uint32_t textureID);

            /**
             * Gets the currently configured skill slot count.
             */
            SkillHUDLayoutConfiguration GetLayoutConfiguration();

            /**
             * Configures the skill HUD to use the layout configuration.
             * @param configuration        The new layout configuration to use.
             */
            void SetLayoutConfiguration(SkillHUDLayoutConfiguration configuration);

    };

}