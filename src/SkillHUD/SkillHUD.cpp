#include "SkillHUD.h"
#include "AttunementSlot.h"
#include "PotionAttunement.h"
#include "SkillSlot.h"
#include "PotionSlot.h"
#include "../Input/SpellCastController.h"
#include "../Input/InputEventController.h"
#include "../Renderer/Renderer.h"
#include <math.h>
#include <nlohmann/json.hpp>
#include <fstream>

// #define TextureDebug
#ifdef TextureDebug
#define TextureLog(...) RE::ConsoleLog::GetSingleton()->Print(__VA_ARGS__)
#else
#define TextureLog(...)
#endif

#define LoadThemeValue(__val__) if (!JSON.contains("__val__") || !JSON["__val__"].is_number_float()) { \
            _theme.__val__ = JSON.value("__val__", 64.0f); \
        }

namespace AttunementSkillbar {

    float EaseInQuartWithTime(float time) {
        return powf(time, 4.0f);
    }

    float EaseOutQuartWithTime(float time) {
        return 1.0f - powf(1.0f - time, 4.0f);
    }

    float EaseInOutQuartWithTime(float time) {
        if (time >= 0.5f) {
            return 1.0f - powf(-2.0f * time + 2.0f, 4.0f) / 2.0f;
        }
        else {
            return 8.0f * powf(time, 4.0f);
        }
    }

    inline float NormalizedTime(float currentTime, float maxTime) {
        return std::clamp(currentTime / maxTime, 0.0f, 1.0f);
    }

    inline float InterpolatedValue(float start, float end, float time) {
        return start + (end - start) * time;
    }

    // MARK: Configuration

    void SkillHUD::ResetState() {
        _activePotionSlot = 0;
        _activeAttunement = 0;
        _previouslyActiveAttunement = 0;
        _attunementChangeTime = -1.0f;
        _showsPotions = false;
        _swappingPotions = false;
        _potionChangeTime = -1.0f;

        _potionIndicator->ResetState();
        for (uint32_t i = 0; i < _configuration.attunementCount; i++) {
            for (uint32_t j = 0; j < _configuration.skillSlotCount; j++) {
                _attunementSlots[i]->_skillSlots[j]->ResetState();
            }
        }

        for (uint32_t i = 0; i < _configuration.skillSlotCount; i++) {
            _potionAttunement->_skillSlots[i]->ResetState();
        }

        {
            std::lock_guard lock(_powerCooldownLock);
            _powerCooldownMap.clear();
            _shoutCooldownTotalTime = 0.0f;
        }
    }

    void SkillHUD::UseDefaultConfiguration() {
        // Delete the current attunement instances
        ReleaseAttunements();
        _configuration = {};

        // Recreate attunements, each with the default configuration and skill slots
        CreateAttunements();
        PrepareLayout();
        
        AssignAttunementKeybind(RE::BSKeyboardDevice::Keys::kF1, 0);
        AssignAttunementKeybind(RE::BSKeyboardDevice::Keys::kF2, 1);
        AssignAttunementKeybind(RE::BSKeyboardDevice::Keys::kF3, 2);
        AssignAttunementKeybind(RE::BSKeyboardDevice::Keys::kF4, 3);

        SetTextureIDForAttunement(0, 20);
        SetTextureIDForAttunement(1, 22);
        SetTextureIDForAttunement(2, 19);
        SetTextureIDForAttunement(3, 18);

        AssignSkillKeybind(RE::BSKeyboardDevice::Keys::kNum1, 0);
        AssignSkillKeybind(RE::BSKeyboardDevice::Keys::kNum2, 1);
        AssignSkillKeybind(RE::BSKeyboardDevice::Keys::kNum3, 2);
        AssignSkillKeybind(RE::BSKeyboardDevice::Keys::kNum4, 3);
        AssignSkillKeybind(RE::BSKeyboardDevice::Keys::kNum5, 4);
        AssignSkillKeybind(RE::BSKeyboardDevice::Keys::kNum6, 5);
        AssignSkillKeybind(RE::BSKeyboardDevice::Keys::kNum7, 6);
        AssignSkillKeybind(RE::BSKeyboardDevice::Keys::kNum8, 7);
        AssignSkillKeybind(RE::BSKeyboardDevice::Keys::kNum9, 8);
        AssignSkillKeybind(RE::BSKeyboardDevice::Keys::kNum0, 9);

        AssignPotionKeybind(RE::BSKeyboardDevice::Keys::kQ);
        
        _hasConfiguration = true;
    }

    void SkillHUD::SetConfiguration(
        SkillHUDConfiguration configuration,
        AttunementSlotConfiguration *attunementConfigurations,
        SkillSlotConfiguration *slotConfigurations
    ) {
        // Delete the current attunement instances
        ReleaseAttunements();
        _configuration = configuration;

        // Recreate the attunements
        CreateAttunements(attunementConfigurations, slotConfigurations);
        PrepareLayout();

        _hasConfiguration = true;
    }

    void SkillHUD::CreateAttunements(
        AttunementSlotConfiguration *configurations,
        SkillSlotConfiguration *skillConfigurations
    ) {
        uint32_t attunementCount = _configuration.attunementCount;
        uint32_t slotCount = _configuration.skillSlotCount;
        _attunementSlots = new AttunementSlot *[_configuration.attunementCount];

        for (uint32_t i = 0; i < attunementCount; i++) {
            AttunementSlot *attunement = new AttunementSlot();
            AttunementSlotConfiguration config = {};
            if (configurations) {
                config = configurations[i];
            }
            config.index = i;
            SkillSlotConfiguration *slotConfig = skillConfigurations ? skillConfigurations + (i * slotCount) : nullptr;

            // This also causes the slots to be created
            attunement->SetConfiguration(this, config, slotConfig);
            _attunementSlots[i] = attunement;
        }

        // Also create the potion attunement and skill indicator
        _potionAttunement = new PotionAttunement();
        _potionAttunement->SetConfiguration(this, {}, nullptr);

        _potionIndicator = new PotionSlotIndicator();
        _potionIndicator->SetConfiguration({}, {});

        _activePotionSlot = 0;
    }

    void SkillHUD::ReleaseAttunements() {
        if (!_attunementSlots) {
            // If attunements were not initalized, there is no action to take
            return;
        }

        delete _potionAttunement;
        _potionAttunement = nullptr;
        _activePotionSlot = 0;

        for (uint32_t i = 0; i < _configuration.attunementCount; i++) {
            delete _attunementSlots[i];
        }

        delete _attunementSlots;
        _attunementSlots = nullptr;

        delete _potionIndicator;
        _potionIndicator = nullptr;
    }

    // MARK: Runtime configuration

    uint32_t SkillHUD::GetAttunementCount() {
        return _configuration.attunementCount;
    }

    void SkillHUD::SetAttunementCount(uint32_t count) {
        if (count == _configuration.attunementCount) {
            return;
        }

        count = std::clamp(count, (uint32_t)1, MaxAttunementSlots);
        _activeAttunement = std::clamp<uint32_t>(_activeAttunement, 0, count - 1);

        uint32_t oldCount = _configuration.attunementCount;
        auto oldAttunements = _attunementSlots;

        _attunementSlots = new AttunementSlot *[count];

        // Copy over as many old attunements as will fit in the new configuration
        _configuration.attunementCount = count;
        for (size_t i = 0; i < count && i < oldCount; i++) {
            _attunementSlots[i] = oldAttunements[i];
        }

        // Delete all attunements that no longer fit
        for (size_t i = count; i < oldCount; i++) {
            delete oldAttunements[i];
        }
        delete oldAttunements;

        // Create new attunements for the new slots
        for (size_t i = oldCount; i < count; i++) {
            auto *newSlot = new AttunementSlot();
            newSlot->SetConfiguration(this, {}, nullptr);
            _attunementSlots[i] = newSlot;

            // Copy over the keybind assignments from the other skill slots
            for (size_t j = 0; j < _configuration.skillSlotCount; j++) {
                newSlot->_skillSlots[j]->SetKeybindTexture(
                    _attunementSlots[0]->_skillSlots[j]->_keybindTexture
                );
            }
        }

        // The layout must be recalculated whenever the number of slots changes
        PrepareLayout();
    }

    uint32_t SkillHUD::GetSkillCount() {
        return _configuration.skillSlotCount;
    }

    void SkillHUD::SetSkillCount(uint32_t count) {
        if (count == _configuration.skillSlotCount) {
            return;
        }

        count = std::clamp(count, (uint32_t)1, MaxSkillSlots);

        _configuration.skillSlotCount = count;
        
        // Ask all attunement slots to resize the number of skills they have
        for (size_t i = 0; i < _configuration.attunementCount; i++) {
            _attunementSlots[i]->Resize(count);
        }
        _potionAttunement->Resize(count);
        if (_activePotionSlot >= count) {
            _activePotionSlot = 0;
        }

        // The layout must be recalculated whenever the number of slots changes
        PrepareLayout();
    }

    uint32_t SkillHUD::GetTextureIDForAttunement(uint32_t index) {
        auto attunementCount = _configuration.attunementCount;
        if (index < 0 || (uint32_t)index >= attunementCount) {
            return 0;
        }

        return _attunementSlots[index]->_configuration.textureID;
    }
    
    void SkillHUD::SetTextureIDForAttunement(uint32_t index, uint32_t textureID) {
        auto attunementCount = _configuration.attunementCount;
        if (index < 0 || (uint32_t)index >= attunementCount) {
            return;
        }

        _attunementSlots[index]->_configuration.textureID = textureID;
        _attunementSlots[index]->_texture = GetTextureForAttunement(textureID);
    }

    SkillHUDLayoutConfiguration SkillHUD::GetLayoutConfiguration() {
        return _configuration;
    }

    void SkillHUD::SetLayoutConfiguration(SkillHUDLayoutConfiguration configuration) {
        (SkillHUDLayoutConfiguration &)_configuration = configuration;
        PrepareLayout();
    }

    SkillSlotSkillConfiguration SkillHUD::GetSkillConfigurationAtIndex(uint32_t index) {
        if (index < 0 || index >= _configuration.skillSlotCount) {
            return {};
        }

        return _attunementSlots[_activeAttunement]->_skillSlots[index]->_configuration;
    }

    void SkillHUD::InitializePotionConfigurationWithIndex(uint32_t index, SkillSlotSkillConfiguration &skill, PotionSlotConfiguration &potion) {
        if (index < 0 || index >= _configuration.skillSlotCount) {
            return;
        }

        auto potionSlot = static_cast<PotionSlot *>(_potionAttunement->_skillSlots[index]);
        skill = potionSlot->_configuration;
        potion = potionSlot->_genericConfiguration;
    }

    void SkillHUD::AssignAttunementKeybind(uint32_t keyID, uint32_t index) {
        if (index < 0 || index >= _configuration.attunementCount) {
            return;
        }

        KeybindTexture keybindTexture {};

        if (keyID) {
            if (_keybindTextureMap.contains(keyID)) {
                keybindTexture = _keybindTextureMap.at(keyID);
            }
            else if (_keybindTextureMap.contains(0xFFFF)) {
                keybindTexture = _keybindTextureMap.at(0xFFFF);
            }
        }

        _attunementSlots[index]->SetKeybindTexture(keybindTexture);
    }

    void SkillHUD::AssignSkillKeybind(uint32_t keyID, uint32_t index) {
        if (index < 0 || index >= _configuration.skillSlotCount) {
            return;
        }

        KeybindTexture keybindTexture {};

        if (keyID) {
            if (_keybindTextureMap.contains(keyID)) {
                keybindTexture = _keybindTextureMap.at(keyID);
            }
            else if (_keybindTextureMap.contains(0xFFFF)) {
                keybindTexture = _keybindTextureMap.at(0xFFFF);
            }
        }

        for (uint32_t i = 0; i < _configuration.attunementCount; i++) {
            _attunementSlots[i]->_skillSlots[index]->SetKeybindTexture(keybindTexture);
        }

        _potionAttunement->_skillSlots[index]->SetKeybindTexture(keybindTexture);
    }

    void SkillHUD::AssignPotionKeybind(uint32_t keyID) {
        KeybindTexture keybindTexture {};

        if (keyID) {
            if (_keybindTextureMap.contains(keyID)) {
                keybindTexture = _keybindTextureMap.at(keyID);
            }
            else if (_keybindTextureMap.contains(0xFFFF)) {
                keybindTexture = _keybindTextureMap.at(0xFFFF);
            }
        }

        _potionIndicator->SetKeybindTexture(keybindTexture);
    }

    void SkillHUD::AssignNextAttunementKeybind(uint32_t keyID) {
        KeybindTexture keybindTexture {};
        if (keyID && _keybindTextureMap.contains(keyID)) {
            keybindTexture = _keybindTextureMap.at(keyID);
        }

        _nextAttunementKeybind = keybindTexture;
    }

    void SkillHUD::AssignPreviousAttunementKeybind(uint32_t keyID) {
        KeybindTexture keybindTexture {};
        if (keyID && _keybindTextureMap.contains(keyID)) {
            keybindTexture = _keybindTextureMap.at(keyID);
        }

        _previousAttunementKeybind = keybindTexture;
    }

    // MARK: Input responders

    void SkillHUD::ActivateAttunement(uint32_t index, bool animated) {
        if (_activeAttunement == index || index >= _configuration.attunementCount) {
            return;
        }

        if (!animated) {
            _activeAttunement = index;
            _previouslyActiveAttunement = index;
            _attunementChangeTime = -1.0f;
        }
        else {
            _previouslyActiveAttunement = _activeAttunement;
            _activeAttunement = index;
            _attunementChangeTime = 0.0f;
        }
    }

    void SkillHUD::ActivateNextAttunement(bool animated) {
        uint32_t nextIndex = _activeAttunement + 1;
        if (nextIndex >= _configuration.attunementCount) {
            nextIndex = 0;
        }
        
        return ActivateAttunement(nextIndex, animated);
    }

    void SkillHUD::ActivatePreviousAttunement(bool animated) {
        if (_activeAttunement == 0) {
            return ActivateAttunement(_configuration.attunementCount - 1, animated);
        }
        else {
            return ActivateAttunement(_activeAttunement - 1, animated);
        }
    }

    void SkillHUD::AssignSkill(SkillSlotSkillConfiguration skill, uint32_t index) {
        AssignSkillForAttunement(skill, _activeAttunement, index);
    }

    void SkillHUD::AssignSkillForAttunement(SkillSlotSkillConfiguration skill, uint32_t attunementIndex, uint32_t index) {
        if (!_attunementSlots) {
            return;
        }

        if (index > _configuration.skillSlotCount) {
            return;
        }

        if (attunementIndex > _configuration.attunementCount) {
            return;
        }

        auto attunement = _attunementSlots[attunementIndex];

        auto skillSlot = attunement->_skillSlots[index];

        SkillSlotConfiguration newConfig = {};
        newConfig.castingSource = skill.castingSource;
        newConfig.index = index;
        newConfig.skillID = skill.skillID;

        skillSlot->SetConfiguration(newConfig);

        // If the skill is a shout, store its id
        if (skill.skillID) {
            auto form = RE::TESForm::LookupByID(skill.skillID);
            if (form && form->GetFormType() == RE::FormType::Shout) {
                std::lock_guard lock(_powerCooldownLock);
                _shoutSet.emplace(skill.skillID);
            }
        }

    }

    void SkillHUD::AssignPotion(SkillSlotSkillConfiguration skill, PotionSlotConfiguration potion, uint32_t index) {
        if (!_attunementSlots) {
            return;
        }

        if (index > _configuration.skillSlotCount) {
            return;
        }

        auto skillSlot = static_cast<PotionSlot *>(_potionAttunement->_skillSlots[index]);

        SkillSlotConfiguration newConfig = {};
        newConfig.castingSource = skill.castingSource;
        newConfig.index = index;
        newConfig.skillID = skill.skillID;

        skillSlot->SetConfiguration(newConfig, potion);

        // Determine the count of this potion
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return;
        }

        uint32_t count = CountPotion(skill, potion);

        skillSlot->SetCount(count);

        // If this was the active potion, also update the potion indicator
        if (index == _activePotionSlot) {
            _potionIndicator->SetConfiguration(newConfig, potion);
            _potionIndicator->SetCount(count);
        }

    }

    void SkillHUD::PressSkillAtIndex(uint32_t index, bool potion) {
        if (index < 0 || index >= _configuration.skillSlotCount) {
            return;
        }

        auto attunement = potion ? _potionAttunement : _attunementSlots[_activeAttunement];
        attunement->_skillSlots[index]->BeginPress();
    }

    void SkillHUD::ReleaseSkillAtIndex(uint32_t index, bool potion) {
        if (index < 0 || index >= _configuration.skillSlotCount) {
            return;
        }

        auto attunement = potion ? _potionAttunement : _attunementSlots[_activeAttunement];
        attunement->_skillSlots[index]->FinishPress();
    }

    void SkillHUD::PressPotionIndicator() {
        _potionIndicator->BeginPress();
    }

    void SkillHUD::ReleasePotionIndicator() {
        _potionIndicator->FinishPress();
    }

    void SkillHUD::SetShowsPotions(bool shows, bool animated) {
        if (shows != _showsPotions) {
            _showsPotions = shows;

            if (animated) {
                _swappingPotions = true;
                _potionChangeTime = 0.0f;
            }
            else {
                _swappingPotions = false;
                _potionChangeTime = 1.0f;
            }
        }
        // If this is invoked as non-animated without changing the current state
        // instantly end any in-progress animation
        else if (!animated && _swappingPotions) {
            _swappingPotions = false;
            _potionChangeTime = 1.0f;
        }
    }

    uint32_t SkillHUD::GetActivePotion() {
        return _activePotionSlot;
    }

    void SkillHUD::SetActivePotion(uint32_t index) {
        if (index < 0 || index >= _configuration.skillSlotCount) {
            return;
        }

        _activePotionSlot = index;
        auto potionSlot = static_cast<PotionSlot *>(_potionAttunement->_skillSlots[index]);
        _potionIndicator->SetConfiguration(potionSlot->_configuration, potionSlot->_genericConfiguration);
        _potionIndicator->SetCount(potionSlot->_count);
    }

    // MARK: Layout and rendering

    void SkillHUD::PrepareLayout() {
        // Determine the scale at which the HUD will render
        float windowScale = std::min(Renderer::GetResolutionScaleHeight(), Renderer::GetResolutionScaleWidth());
        float baseScale = windowScale * _configuration.sizeScale;

        bool showsPotion = _configuration.showsPotion;

        const auto &theme = Theme();

        // Determine the maximum width at this scale and adjust to fit within the requested margins
        float totalWidth = std::max(
            baseScale * (
                theme.SkillSlotSizeBase * _configuration.skillSlotCount + // Skill slot width
                theme.SkillSlotSpacingBase * (_configuration.skillSlotCount - 1) // Skill slot spacing width
            ),
            baseScale * (
                theme.AttunementSlotSizeDeselected * (_configuration.attunementCount - 1) + // Deselected attunement width
                theme.AttunementSlotSizeDeselected * theme.AttunementSlotSelectedScale + // Selected attunement width
                theme.AttunementSlotSpacingBase * (_configuration.attunementCount - 1) // Attunement spacing width
            )
        ) + (showsPotion ? (theme.SkillSlotSizeBase + theme.PotionSkillGap) * baseScale : 0.0f);

        float screenWidth = ImGui::GetIO().DisplaySize.x;
        float screenHeight = ImGui::GetIO().DisplaySize.y;
        float availableWidth = 
            screenWidth -
            windowScale * (_configuration.marginLeft + _configuration.marginRight);

        float HUDScale = baseScale;
        if (totalWidth > availableWidth) {
            HUDScale *= availableWidth / totalWidth;
            totalWidth = availableWidth;
        }

        float totalHeight = HUDScale * (theme.AttunementSlotSizeDeselected * theme.AttunementSlotSelectedScale + theme.SkillSlotSizeBase + theme.SkillAttunementGap);

        // Then determine the x and y point at which drawing will start
        float x, y;
        switch (_configuration.horizontalAnchor) {
            case SkillHUDConfigurationAnchor::Start:
                x = _configuration.marginLeft * windowScale + _configuration.anchorPoint.x * windowScale;
                break;
            case SkillHUDConfigurationAnchor::Center:
                x = _configuration.anchorPoint.x * windowScale + (screenWidth - totalWidth) / 2;
                break;
            case SkillHUDConfigurationAnchor::End:
                x = screenWidth - _configuration.marginRight * windowScale - totalWidth - _configuration.anchorPoint.x * windowScale;
                break;
        }

        switch (_configuration.verticalAnchor) {
            case SkillHUDConfigurationAnchor::Start:
                y = _configuration.anchorPoint.y * windowScale;
                break;
            case SkillHUDConfigurationAnchor::Center:
                y = _configuration.anchorPoint.y * windowScale + (screenHeight - totalHeight) / 2;
                break;
            case SkillHUDConfigurationAnchor::End:
                y = screenHeight - totalHeight - _configuration.anchorPoint.y * windowScale;
                break;
        }
        
        x = std::clamp(
            x,
            _configuration.marginLeft * windowScale,
            screenWidth - _configuration.marginRight * windowScale - totalWidth
        );
        y = std::clamp(y, 0.0f, screenHeight - totalHeight);

        // Store the computed values and use them for drawing
        _scale = HUDScale;
        _position = ImVec2 { x, y };
        _skillPosition = ImVec2 { x, y + totalHeight - HUDScale * theme.SkillSlotSizeBase };

        if (showsPotion) {
            _potionPosition = { _skillPosition.x, _skillPosition.y };
            _position.x += HUDScale * (theme.SkillSlotSizeBase + theme.PotionSkillGap);
            _skillPosition.x += HUDScale * (theme.SkillSlotSizeBase + theme.PotionSkillGap);

            _potionIndicator->_scale = HUDScale;
            _potionIndicator->_position = _potionPosition;
        }

        // Since skill slots don't move, pre-assign their positions now
        for (size_t i = 0; i < _configuration.attunementCount; i++) {
            auto attunement = _attunementSlots[i];
            float skillPositionX = _skillPosition.x;

            for (size_t j = 0; j < _configuration.skillSlotCount; j++) {
                auto slot = attunement->_skillSlots[j];
                slot->_position = ImVec2 { skillPositionX, _skillPosition.y };
                slot->_scale = HUDScale;
                skillPositionX += HUDScale * (theme.SkillSlotSizeBase + theme.SkillSlotSpacingBase);
            }
        }

        float skillPositionX = _skillPosition.x;
        for (size_t i = 0; i < _configuration.skillSlotCount; i++) {
            auto slot = _potionAttunement->_skillSlots[i];
            slot->_position = ImVec2 { skillPositionX, _skillPosition.y };
            slot->_scale = HUDScale;
            skillPositionX += HUDScale * (theme.SkillSlotSizeBase + theme.SkillSlotSpacingBase);
        }
    }

    void SkillHUD::Render() {
        // If the skill HUD has not been initialized, do not render
        if (!_attunementSlots) {
            return;
        }

        auto delta = ImGui::GetIO().DeltaTime;

        AdvanceCooldowns();
        
        const auto &theme = Theme();

        if (_activeAttunement != _previouslyActiveAttunement) {
            _attunementChangeTime += delta;
        }

        if (_swappingPotions) {
            _potionChangeTime += delta;
        }

        float attunementOffset = _scale * (theme.AttunementSlotSizeDeselected * (theme.AttunementSlotSelectedScale - 1));

        if (_showsPotions || _swappingPotions) {
            RenderPotions(delta, attunementOffset);
        }
        else {
            RenderAttunements(delta, attunementOffset);
        }

        // Render the potion slot at its regular position
        if (_configuration.showsPotion) {
            _potionIndicator->Render(delta, 1.0f, _translateY, _opacity);
        }
    }

    void SkillHUD::AdvanceAnimations() {
        auto delta = ImGui::GetIO().DeltaTime;
        const auto &theme = Theme();

        if (_activeAttunement != _previouslyActiveAttunement) {
            _attunementChangeTime += delta;

            if (_attunementChangeTime >= theme.AttunementChangeAnimationDuration) {
                _previouslyActiveAttunement = _activeAttunement;
                _attunementChangeTime = -1.0f;
            }
        }

        if (_swappingPotions) {
            _potionChangeTime += delta;

            if (_potionChangeTime >= theme.AttunementChangeAnimationDuration) {
                _swappingPotions = false;
                _potionChangeTime = -1.0f;
            }
        }

        _potionIndicator->AdvanceAnimations(delta);
        auto size = _configuration.skillSlotCount;
        auto attunementSize = _configuration.attunementCount;
        
        for (decltype(size) i = 0; i < size; i++) {
            _potionAttunement->_skillSlots[i]->AdvanceAnimations(delta);

            for (decltype(size) j = 0; j < attunementSize; j++) {
                _attunementSlots[j]->_skillSlots[i]->AdvanceAnimations(delta);
            }
        }
    }

    void SkillHUD::RenderAttunementCycleKeybinds(float, float attunementOffset, float scaleY) {
        const auto &theme = Theme();

        float keybindHeight = theme.AttunementSlotKeybindHeightBase * _scale * theme.AttunementSlotSelectedScale;
        auto color = 0x00FFFFFF | ((uint32_t)(_opacity * 255) << 24);
        auto drawList = ImGui::GetWindowDrawList();
        
        if (_nextAttunementKeybind.textureID) {
            float width = _nextAttunementKeybind.sizeRatio * keybindHeight;
            ImVec2 location = {
                _position.x +
                    theme.AttunementSlotSizeDeselected * _scale * _configuration.attunementCount +
                    _scale * (theme.AttunementSlotSelectedScale - 1) * theme.AttunementSlotSizeDeselected +
                    _scale * (theme.AttunementSlotSelectedScale - 1) * theme.AttunementSlotSpacingBase +
                    theme.AttunementCycleKeybindGap * _scale,
                _position.y + attunementOffset +
                    theme.AttunementSlotSizeDeselected / 2.0f -
                    (keybindHeight * scaleY) / 2.0f +
                    _translateY
            };

            ImVec2 bottomRight = {
                location.x + width,
                location.y + keybindHeight * scaleY
            };

            drawList->AddImage(
                _nextAttunementKeybind.textureID,
                location, bottomRight,
                _nextAttunementKeybind.topLeft, _nextAttunementKeybind.bottomRight,
                color
            );
        }

        if (_previousAttunementKeybind.textureID) {
            float width = _previousAttunementKeybind.sizeRatio * keybindHeight;
            ImVec2 location = {
                _position.x - width - theme.AttunementCycleKeybindGap * _scale,
                _position.y + attunementOffset +
                    theme.AttunementSlotSizeDeselected / 2.0f -
                    (keybindHeight * scaleY) / 2.0f +
                    _translateY
            };

            ImVec2 bottomRight = {
                location.x + width,
                location.y + keybindHeight * scaleY
            };

            drawList->AddImage(
                _previousAttunementKeybind.textureID,
                location, bottomRight,
                _previousAttunementKeybind.topLeft, _previousAttunementKeybind.bottomRight,
                color
            );
        }
    }

    void SkillHUD::RenderAttunements(float delta, float attunementOffset) {
        const auto &theme = Theme();
        float attunementX = _position.x;

        // Determine the point at which the attunement change animation is if it is running
        float normalizedTime = NormalizedTime(_attunementChangeTime, theme.AttunementChangeAnimationDuration);
        normalizedTime = EaseInOutQuartWithTime(normalizedTime);

        // If the animation ended, reset the animation arguments
        if (normalizedTime >= 1) {
            // Also cancel out any pressing state on the outgoing attunment's skill slots
            auto attunement = _attunementSlots[_previouslyActiveAttunement];
            for (size_t i = 0; i < _configuration.skillSlotCount; i++) {
                attunement->_skillSlots[i]->CancelPress();
            }

            normalizedTime = 1;
            _attunementChangeTime = -1.0f;
            _previouslyActiveAttunement = _activeAttunement;
        }

        // Draw the cycle attunement keybinds if defined
        if (_configuration.attunementCount > 1) {
            RenderAttunementCycleKeybinds(delta, attunementOffset);
        }

        // Begin drawing each attunement starting from the saved drawing position
        for (size_t i = 0; i < _configuration.attunementCount; i++) {
            auto attunement = _attunementSlots[i];

            if (i == _activeAttunement) {
                // If the active attunement is in the process of changing, apply the appropriate animation
                if (_activeAttunement != _previouslyActiveAttunement) {
                    auto scale = InterpolatedValue(_scale, _scale * theme.AttunementSlotSelectedScale, normalizedTime);
                    attunement->_scale = scale;
                    attunement->_position = {
                        attunementX,
                        _position.y + theme.AttunementSlotSizeDeselected * (_scale * theme.AttunementSlotSelectedScale - scale)
                    };
                    attunementX += scale * theme.AttunementSlotSizeDeselected + _scale * theme.AttunementSlotSpacingBase;

                    // In the active flipping animation, only render the active attunement's slots
                    // past the halfway point in the animation
                    if (normalizedTime > 0.5f) {
                        auto scaleY = InterpolatedValue(0.0f, 1.0f, (normalizedTime - 0.5f) * 2);
                        RenderAttunementSlotSkills(attunement, delta, scaleY);
                    }
                    else {
                        AdvanceAttunementSlotAnimations(attunement, delta);
                    }
                }
                else {
                    attunement->_scale = _scale * theme.AttunementSlotSelectedScale;
                    attunement->_position = { attunementX, _position.y };
                    attunementX += _scale * theme.AttunementSlotSelectedScale * theme.AttunementSlotSizeDeselected + _scale * theme.AttunementSlotSpacingBase;

                    // For the selected attunement, also draw its skills
                    RenderAttunementSlotSkills(attunement, delta, 1.0f);
                }

            }
            else if (i == _previouslyActiveAttunement) {
                // This branch is only reached during an active animation
                auto scale = InterpolatedValue(_scale * theme.AttunementSlotSelectedScale, _scale, normalizedTime);
                attunement->_scale = scale;
                attunement->_position = {
                    attunementX,
                    _position.y + theme.AttunementSlotSizeDeselected * (_scale * theme.AttunementSlotSelectedScale - scale)
                };
                attunementX += scale * theme.AttunementSlotSizeDeselected + _scale * theme.AttunementSlotSpacingBase;

                // In the active flipping animation, only render the previous attunement's slots
                // before the halfway point in the animation
                if (normalizedTime < 0.5f) {
                    auto scaleY = InterpolatedValue(1.0f, 0.0f, normalizedTime * 2);
                    RenderAttunementSlotSkills(attunement, delta, scaleY);
                }
                else {
                    AdvanceAttunementSlotAnimations(attunement, delta);
                }
            }
            else {
                attunement->_scale = _scale;
                attunement->_position = { attunementX, _position.y + attunementOffset };
                attunementX += _scale * (theme.AttunementSlotSizeDeselected + theme.AttunementSlotSpacingBase);

                // For deselected attunements, advance any animations
                AdvanceAttunementSlotAnimations(attunement, delta);
            }

            attunement->Render(delta, 1.0f, _translateY, _opacity);
        }
    }

    void SkillHUD::RenderPotions(float delta, float attunementOffset) {
        const auto &theme = Theme();
        float attunementX = _position.x;

        // Determine the point at which the potion change animation is if it is running
        float normalizedTime = NormalizedTime(_potionChangeTime, theme.AttunementChangeAnimationDuration);
        normalizedTime = EaseInOutQuartWithTime(normalizedTime);

        // If the animation ended, reset the animation arguments
        if (normalizedTime >= 1) {
            // Also cancel out any pressing state on the potion slot or outgoing attunement skill slots
            auto attunement = _showsPotions ? _attunementSlots[_activeAttunement] : _potionAttunement;
            for (size_t i = 0; i < _configuration.skillSlotCount; i++) {
                attunement->_skillSlots[i]->CancelPress();
            }

            normalizedTime = 1;
            _potionChangeTime = -1.0f;
            _swappingPotions = false;
        }

        float attunementScale = 1.0f;
        bool rendersAttunements = false;
        if (_swappingPotions) {
            if (_showsPotions) {
                rendersAttunements = normalizedTime < 0.5f;
                attunementScale = std::clamp(InterpolatedValue(1.0f, 0.0f, normalizedTime * 2.0f), 0.0f, 1.0f);
            }
            else {
                rendersAttunements = normalizedTime >= 0.5f;
                attunementScale = std::clamp(InterpolatedValue(0.0f, 1.0f, (normalizedTime - 0.5f) * 2.0f), 0.0f, 1.0f);
            }
        }
        else {
            rendersAttunements = !_showsPotions;
        }

        // Begin drawing each attunement starting from the saved drawing position
        if (rendersAttunements) {
            
            if (_configuration.attunementCount > 1) {
                RenderAttunementCycleKeybinds(delta, attunementOffset, attunementScale);
            }

            for (size_t i = 0; i < _configuration.attunementCount; i++) {
                auto attunement = _attunementSlots[i];

                if (i == _activeAttunement && !_showsPotions) {
                    attunement->_scale = _scale * theme.AttunementSlotSelectedScale;
                    attunement->_position = { attunementX, _position.y };
                    attunementX += _scale * theme.AttunementSlotSelectedScale * theme.AttunementSlotSizeDeselected + _scale * theme.AttunementSlotSpacingBase;

                    // In the active flipping animation, only render the active attunement's slots
                    // past the halfway point in the animation
                    if (normalizedTime > 0.5f) {
                        auto scaleY = InterpolatedValue(0.0f, 1.0f, (normalizedTime - 0.5f) * 2);
                        RenderAttunementSlotSkills(attunement, delta, scaleY);
                    }
                    else {
                        AdvanceAttunementSlotAnimations(attunement, delta);
                    }
                    

                }
                else if (i == _activeAttunement && _showsPotions) {
                    attunement->_scale = _scale * theme.AttunementSlotSelectedScale;
                    attunement->_position = { attunementX, _position.y };
                    attunementX += _scale * theme.AttunementSlotSelectedScale * theme.AttunementSlotSizeDeselected + _scale * theme.AttunementSlotSpacingBase;

                    // In the active flipping animation, only render the previous attunement's slots
                    // before the halfway point in the animation
                    if (normalizedTime < 0.5f) {
                        auto scaleY = InterpolatedValue(1.0f, 0.0f, normalizedTime * 2);
                        RenderAttunementSlotSkills(attunement, delta, scaleY);
                    }
                    else {
                        AdvanceAttunementSlotAnimations(attunement, delta);
                    }
                }
                else {
                    attunement->_scale = _scale;
                    attunement->_position = { attunementX, _position.y + attunementOffset };
                    attunementX += _scale * (theme.AttunementSlotSizeDeselected + theme.AttunementSlotSpacingBase);

                    // For deselected attunements, advance any animations
                    AdvanceAttunementSlotAnimations(attunement, delta);
                }

                attunement->Render(delta, attunementScale, _translateY, _opacity);
            }
        }
        else {
            for (size_t i = 0; i < _configuration.attunementCount; i++) {
                auto attunement = _attunementSlots[i];
                AdvanceAttunementSlotAnimations(attunement, delta);
            }
        }

        // Then render the potion attunement's skills
        if (_swappingPotions) {
            // During the animation, based on whether the HUD is swapping to or from potions
            // only show the potion skills in the second or first part of the animation
            if (!_showsPotions) {
                if (normalizedTime < 0.5f) {
                    auto scaleY = InterpolatedValue(1.0f, 0.0f, normalizedTime * 2);
                    RenderAttunementSlotSkills(_potionAttunement, delta, scaleY);
                }
                else {
                    AdvanceAttunementSlotAnimations(_potionAttunement, delta);
                }
            }
            else {
                if (normalizedTime > 0.5f) {
                    auto scaleY = InterpolatedValue(0.0f, 1.0f, (normalizedTime - 0.5f) * 2);
                    RenderAttunementSlotSkills(_potionAttunement, delta, scaleY);
                }
                else {
                    AdvanceAttunementSlotAnimations(_potionAttunement, delta);
                }
            }
        }
        else if (_showsPotions) {
            // Outside of the animation, render the skills directly
            RenderAttunementSlotSkills(_potionAttunement, delta, 1.0f);
        }
    }

    inline void SkillHUD::RenderAttunementSlotSkills(AttunementSlot *attunement, float delta, float scaleY) {
        SkillSlot *assigningSlot = nullptr;
        for (size_t j = 0; j < _configuration.skillSlotCount; j++) {
            auto slot = attunement->_skillSlots[j];
            // The slot that is being assigned to in the assigning HUD is drawn last to have a higher Z index
            if (slot->_assignmentProgress != -1.0f && !assigningSlot) {
                assigningSlot = slot;
                continue;
            }
            slot->Render(delta, scaleY, _translateY, _opacity);
        }

        if (assigningSlot) {
            assigningSlot->Render(delta, scaleY, _translateY, _opacity);
        }
    }

    inline void SkillHUD::AdvanceAttunementSlotAnimations(AttunementSlot *attunement, float delta) {
        for (size_t j = 0; j < _configuration.skillSlotCount; j++) {
            auto slot = attunement->_skillSlots[j];
            slot->AdvanceAnimations(delta);
        }
    }

    // MARK: Alchemy count tracking

    /**
     * Used to handle an issue where the process event is called twice for a single
     * potion use.
     */
    bool _potionCountDisabled = false;

    RE::BSEventNotifyControl SkillHUD::ProcessEvent(const RE::TESContainerChangedEvent* event, RE::BSTEventSource<RE::TESContainerChangedEvent>*) {
        // Update the count of alchemy and scroll items based on the change
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player || !event) {
            return RE::BSEventNotifyControl::kContinue;
        }
        bool playerInvolved = (
            event->newContainer == player->GetFormID() || 
            event->oldContainer == player->GetFormID()
        );
        
        if (!playerInvolved) {
            return RE::BSEventNotifyControl::kContinue;
        }

        // Only react to alchemy items and scrolls
        auto *form = RE::TESForm::LookupByID(event->baseObj);
        if (!form) {
            return RE::BSEventNotifyControl::kContinue;
        }

        bool isPotion = form->GetFormType() == RE::FormType::AlchemyItem;
        bool isScroll = form->GetFormType() == RE::FormType::Scroll;

        if (!isPotion && !isScroll) {
            return RE::BSEventNotifyControl::kContinue;
        }

        if (_potionCountDisabled) {
            _potionCountDisabled = false;
            return RE::BSEventNotifyControl::kContinue;
        }

        // Disable the next update of this kind on the same frame
        if (event->itemCount) {
            _potionCountDisabled = true;
            SKSE::GetTaskInterface()->AddTask([]() {
                _potionCountDisabled = false;
            });
        }

        if (isScroll) {
            auto scrollItem = form->As<RE::ScrollItem>();
            bool playerGained = (event->newContainer == player->GetFormID());
            int32_t change = playerGained ? event->itemCount : -event->itemCount;
            
            for (size_t i = 0; i < _configuration.skillSlotCount; i++) {
                auto potionSlot = static_cast<PotionSlot *>(_potionAttunement->_skillSlots[i]);
                auto slotKind = potionSlot->_genericConfiguration.kind;
                if (slotKind != PotionSlotItemKind::Scroll) {
                    continue;
                }

                bool adjusted = false;
                if (potionSlot->_configuration.skillID == scrollItem->GetFormID()) {
                    adjusted = true;
                    potionSlot->AdjustCount(change);

                    if (i == _activePotionSlot) {
                        _potionIndicator->AdjustCount(change);
                    }
                }
            }
        }
        else if (isPotion) {
            auto alchemyItem = form->As<RE::AlchemyItem>();
            auto config = InputEventController::PotionConfigurationWithAlchemyItem(alchemyItem);
            bool playerGained = (event->newContainer == player->GetFormID());
            int32_t change = playerGained ? event->itemCount : -event->itemCount;
            
            for (size_t i = 0; i < _configuration.skillSlotCount; i++) {
                auto potionSlot = static_cast<PotionSlot *>(_potionAttunement->_skillSlots[i]);
                auto slotKind = potionSlot->_genericConfiguration.kind;
                if (slotKind == PotionSlotItemKind::None || slotKind == PotionSlotItemKind::Scroll) {
                    continue;
                }

                bool adjusted = false;
                if (
                    potionSlot->_configuration.skillID == 0 &&
                    InputEventController::PotionMagnitudeForConfiguration(alchemyItem, potionSlot->_genericConfiguration) > 0.0f
                ) {
                    adjusted = true;
                    potionSlot->AdjustCount(change);
                }
                else if (potionSlot->_configuration.skillID == form->formID) {
                    adjusted = true;
                    potionSlot->AdjustCount(change);
                }

                if (i == _activePotionSlot && adjusted) {
                    _potionIndicator->AdjustCount(change);
                }
            }
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    uint32_t SkillHUD::CountPotion(SkillSlotSkillConfiguration skill, PotionSlotConfiguration potion) {
        int32_t count = 0;

        if (potion.kind == PotionSlotItemKind::None) {
            return count;
        }

        auto player = RE::PlayerCharacter::GetSingleton();

        auto inventory = player->GetInventory();

        if (potion.kind == PotionSlotItemKind::Scroll) {
            for (auto &[object, data] : inventory) {
                if (object->GetFormType() != RE::FormType::Scroll) {
                    continue;
                }

                if (object->formID == skill.skillID) {
                    count += data.first;
                }
            }
        }
        else {
            for (auto &[object, data] : inventory) {
                if (object->GetFormType() != RE::FormType::AlchemyItem) {
                    continue;
                }

                if (skill.skillID != 0) {
                    if (object->formID == skill.skillID) {
                        count += data.first;
                    }
                }
                else {
                    if (InputEventController::PotionMagnitudeForConfiguration(object->As<RE::AlchemyItem>(), potion) > 0.0f) {
                        count += data.first;
                    }
                }
            }
        }

        return count;
    }

    void SkillHUD::UpdateScrollCount(RE::FormID scrollID) {
        auto count = -1;

        for (uint32_t i = 0; i < _configuration.skillSlotCount; i++) {
            auto slot = static_cast<PotionSlot *>(_potionAttunement->_skillSlots[i]);

            if (slot->_configuration.skillID == scrollID) {
                if (count < 0) {
                    count = CountPotion(slot->_configuration, slot->_genericConfiguration);
                }

                slot->SetCount(count);

                if (i == _activePotionSlot) {
                    _potionIndicator->SetCount(count);
                }
            }
        }
    }
    

    // MARK: Skill cooldown tracking

    RE::BSEventNotifyControl SkillHUD::ProcessEvent(const RE::TESSpellCastEvent* event, RE::BSTEventSource<RE::TESSpellCastEvent>*) {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player || !event || !event->object || (event->object != player)) {
            return RE::BSEventNotifyControl::kContinue;
        }
        
        auto form = RE::TESForm::LookupByID(event->spell);
        if (!form) {
            return RE::BSEventNotifyControl::kContinue;
        }

        if (form->GetFormType() != RE::FormType::Spell) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto spell = form->As<RE::SpellItem>();
        if (!spell || spell->GetSpellType() != RE::MagicSystem::SpellType::kPower) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto now = RE::Calendar::GetSingleton()->GetCurrentGameTime();
        {
            std::lock_guard lock(_powerCooldownLock);
            SkillCooldown cooldown { now, 1.0f };
            _powerCooldownMap.emplace(event->spell, cooldown);
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    float SkillHUD::CooldownForSkill(RE::FormID skill) {
        std::lock_guard lock(_powerCooldownLock);

        // If the skill is a shout, return the global shout cooldown
        if (_shoutSet.contains(skill)) {
            auto player = RE::PlayerCharacter::GetSingleton();
            if (!player) {
                return 0.0f;
            }

            auto process = player->GetActorRuntimeData().currentProcess;
            if (!process) {
                return 0.0f;
            }

            auto highProcess = process->high;
            if (!highProcess) {
                return 0.0f;
            }

            auto voiceCooldown = highProcess->voiceRecoveryTime;

            // If the voice cooldown is non 0 but the total time was not initalized, initialize
            // it now to the cooldown time and start tracking
            if (voiceCooldown > 0.0f) {
                if (_shoutCooldownTotalTime == 0.0f) {
                    _shoutCooldownTotalTime = voiceCooldown;
                }
                return voiceCooldown / _shoutCooldownTotalTime;
            }
            // When the cooldown finishes, also reset the total time
            else {
                if (_shoutCooldownTotalTime != 0.0f) {
                    _shoutCooldownTotalTime = 0.0f;
                }
            }

            return 0.0f;
        }
        // Else if the skill is a power, get its tracked cooldown
        else if (_powerCooldownMap.contains(skill)) {
            return _powerCooldownMap.at(skill).remaining;
        }

        return 0.0f;
    }

    void SkillHUD::AdvanceCooldowns() {
        std::lock_guard lock(_powerCooldownLock);
        float now = RE::Calendar::GetSingleton()->GetCurrentGameTime();
        for (auto i = _powerCooldownMap.begin(); i != _powerCooldownMap.end();) {
            auto &cooldown = i->second;

            // The total cooldown for powers is 1.0f (1 game day)
            cooldown.remaining = 1.0f - (now - cooldown.start);

            if (cooldown.remaining < 0.0f) {
                i = _powerCooldownMap.erase(i);
            }
            else {
                i++;
            }
        }
    }

    void SkillHUD::SetCooldownData(float shoutCooldown, std::unordered_map<RE::FormID, SkillCooldown> powerCooldowns) {
        std::lock_guard lock(_powerCooldownLock);

        _shoutCooldownTotalTime = shoutCooldown;
        _powerCooldownMap = powerCooldowns;
    }

    // MARK: Texture lookup

    SkillTexture SkillHUD::GetTextureForAttunement(uint32_t ID) {
        if (_skillTextureMap.contains(ID)) {
            return _skillTextureMap.at(ID);
        }
        else {
            return emptyButtonTexture;
        }
    }
    SkillTexture SkillHUD::GetTextureForGenericPotion(uint32_t ID) {
        if (_genericPotionTextureMap.contains(ID)) {
            return _genericPotionTextureMap.at(ID);
        }
        else {
            return emptyButtonTexture;
        }
    }

    SkillTexture SkillHUD::GetTextureForSkill(uint32_t ID) {
        if (_skillTextureMap.contains(ID)) {
            TextureLog("Found texture for ID %d", ID);
            return _skillTextureMap.at(ID);
        }
        else {
            TextureLog("Did not find texture for ID %d", ID);
        }

        // If a specific skill icon has not been set, use a generic icon
        // based on the spell school or skill type
        RE::TESForm *form = RE::TESForm::LookupByID(ID);
        if (!form) {
            return emptyButtonTexture;
        }

        if (form->formType == RE::FormType::Spell) {
            auto spell = form->As<RE::SpellItem>();

            switch (spell->GetSpellType()) {
                case RE::MagicSystem::SpellType::kPower:
                    [[fallthrough]];
                case RE::MagicSystem::SpellType::kLesserPower:
                    [[fallthrough]];
                case RE::MagicSystem::SpellType::kAbility:
                    if (_skillTextureMap.contains(25)) {
                        return _skillTextureMap.at(25);
                    }
                    else {
                        return emptyButtonTexture;
                    }
                case RE::MagicSystem::SpellType::kSpell:
                    // For spells, use an icon based on the spell school
                    auto skill = static_cast<uint32_t>(spell->GetAssociatedSkill());
                    if (_skillTextureMap.contains(skill)) {
                        return _skillTextureMap.at(skill);
                    }
                    else {
                        return emptyButtonTexture;
                    }
                    break;
            }
        }
        else if (form->formType == RE::FormType::Shout) {
            if (_skillTextureMap.contains(23)) {
                return _skillTextureMap.at(23);
            }
            else {
                return emptyButtonTexture;
            }
        }

        return emptyButtonTexture;
    }

    SkillTexture SkillHUD::GetSkillSlotBorderTexture() {
        return buttonTexture;
    }

    SkillTexture SkillHUD::GetEmptySkillSlotTexture() {
        return emptyButtonTexture;
    }

    SkillTexture SkillHUD::GetCastingSourceIndicatorTexture(RE::MagicSystem::CastingSource source) {
        switch (source) {
            case RE::MagicSystem::CastingSource::kLeftHand:
                return leftHandTexture;
            case RE::MagicSystem::CastingSource::kRightHand:
                return rightHandTexture;
            default:
                return { nullptr };
        }
    }

    // MARK: Skill Texture loading

    void SkillHUD::Initialize() {

        auto library = std::filesystem::path("Data") / "SkillIcons";
        std::error_code err;

        if (!std::filesystem::exists(library, err) || !std::filesystem::is_directory(library)) {
            logger::info("Skipping SkillHUD initialization because {} does not exist", library.string());
            return;
        }

        for (const auto &file : std::filesystem::directory_iterator(library, err)) {
            if (err) {
                logger::error("Could not enumerate texture files - error: {}", err.message().c_str());
                break;
            }

            if (!file.is_regular_file(err)) {
                continue;
            }

            const auto &path = file.path();

            // The button textures are directly png files, load them here
            if (path.filename().string() == "ButtonTexture.png") {

                ID3D11ShaderResourceView *textureID;
                int32_t width, height;
                bool loaded = Renderer::LoadTextureFromFile(
                    path.string().c_str(),
                    &textureID,
                    width,
                    height
                );

                if (!loaded) {
                    logger::error("Could not load texture file {}", path.string());
                    return;
                }

                buttonTexture.textureID = textureID;
                buttonTexture.bottomRight = ImVec2 { 1.0f, 1.0f };
                continue;
            }
            else if (path.filename().string() == "EmptyButtonTexture.jpg") {

                ID3D11ShaderResourceView *textureID;
                int32_t width, height;
                bool loaded = Renderer::LoadTextureFromFile(
                    path.string().c_str(),
                    &textureID,
                    width,
                    height
                );

                if (!loaded) {
                    logger::error("Could not load texture file {}", path.string());
                    return;
                }

                emptyButtonTexture.textureID = textureID;
                emptyButtonTexture.bottomRight = ImVec2 { 1.0f, 1.0f };
                continue;
            }
            else if (path.filename().string() == "HandIndicatorLeft.png") {
                ID3D11ShaderResourceView *textureID;
                int32_t width, height;
                bool loaded = Renderer::LoadTextureFromFile(
                    path.string().c_str(),
                    &textureID,
                    width,
                    height
                );

                if (!loaded) {
                    logger::error("Could not load texture file {}", path.string());
                    return;
                }

                leftHandTexture.textureID = textureID;
                leftHandTexture.bottomRight = ImVec2 { 1.0f, 1.0f };
                continue;
            }
            else if (path.filename().string() == "HandIndicatorRight.png") {
                ID3D11ShaderResourceView *textureID;
                int32_t width, height;
                bool loaded = Renderer::LoadTextureFromFile(
                    path.string().c_str(),
                    &textureID,
                    width,
                    height
                );

                if (!loaded) {
                    logger::error("Could not load texture file {}", path.string());
                    return;
                }

                rightHandTexture.textureID = textureID;
                rightHandTexture.bottomRight = ImVec2 { 1.0f, 1.0f };
                continue;
            }
            else if (path.filename().string() == "Keybinds.json") {
                LoadKeybindTexturesFromFile(path);
                continue;
            }
            else if (path.filename().string() == "Theme.json") {
                LoadThemeFromFile(path);
                continue;
            }
            else if (path.extension() != ".json") {
                continue;
            }

            LoadIconTexturesFromFile(path);
        }
    }

    void LoadOverridesFromJSON(const nlohmann::json &JSON, const std::string filepath);

    void SkillHUD::LoadIconTexturesFromFile(const std::filesystem::path &path) {
        std::string filepath = path.string();

        std::ifstream inputStream(path);
        auto JSON = nlohmann::json::parse(inputStream);

        // If this is an override json, send it to the spell cast controller controller to parse
        if (JSON.contains("overrides")) {
            return LoadOverridesFromJSON(JSON, filepath);
        }

        // Perform some basic validation on the JSON
        if (!JSON.contains("texturePath") || !JSON["texturePath"].is_string()) {
            logger::error("File {} did not specify a texture, skipping", filepath);
            return;
        }

        if (!JSON.contains("iconSize") || !JSON["iconSize"].is_number_unsigned()) {
            logger::error("File {} does not specify a valid icon size", filepath);
            return;
        }

        if (!JSON.contains("skills") || !JSON["skills"].is_array() || JSON["skills"].size() == 0) {
            logger::error("File {} does not contain any icons", filepath);
            return;
        }

        // First attempt to load the texture and retain a reference to it
        std::string texturePath = JSON.value("texturePath", std::string{});
        if (texturePath == "") {
            logger::error("File {} did not specify a texture, skipping", filepath);
            return;
        }

        ID3D11ShaderResourceView *textureID;
        int32_t width, height;
        bool loaded = Renderer::LoadTextureFromFile(
            texturePath.c_str(),
            &textureID,
            width,
            height
        );

        if (!loaded) {
            logger::error("File {} specified a texture \"{}\" that could not be loaded, skipping", filepath, texturePath);
            return;
        }

        // Get the icon size and ensure the image is a multiple of the icon size in both dimensions
        uint32_t iconSize = JSON.value("iconSize", (uint32_t)0);
        if (iconSize <= 0) {
            logger::error("File {} does not specify a valid icon size", filepath);
            return;
        }
        if (width % iconSize != 0 || height % iconSize != 0) {
            logger::error("File {} specified texture size {}x{} is not a multiple of icon size {}", filepath, width, height, iconSize);
            return;
        }

        float xMax = (float)width / (float)iconSize;
        float yMax = (float)height / (float)iconSize;

        // Enumerate all the skill icon references and create and store them in the map
        const auto &skills = JSON["skills"];
        const auto size = skills.size();
        for (size_t i = 0; i < size; i++) {
            const auto &skill = skills[i];

            // Skill must have either an editor ID or a local ID
            bool hasLocalID = true;
            bool hasEditorID = true;
            if (!skill.contains("editorID") || !skill["editorID"].is_string()) {
                hasEditorID = false;
            }
            if (!skill.contains("ID") || !skill["ID"].is_number_unsigned()) {
                hasLocalID = false;
            }

            if (!hasLocalID && !hasEditorID) {
                logger::error("File {} did not specify a valid ID for skill {}, skipping skill", filepath, i);
                continue;
            }

            // Skill must have package, X and Y
            std::string package = skill.value("package", std::string{});
            if (!skill.contains("package") || !skill["package"].is_string()) {
                logger::error("File {} did not specify a valid position for skill {}, skipping skill", filepath, i);
                continue;
            }

            if (!skill.contains("x") || !skill["x"].is_number_unsigned()) {
                logger::error("File {} did not specify a valid position for skill {}, skipping skill", filepath, i);
                continue;
            }

            if (!skill.contains("y") || !skill["y"].is_number_unsigned()) {
                logger::error("File {} did not specify a valid position for skill {}, skipping skill", filepath, i);
                continue;
            }

            float x = skill.value("x", (float)0);
            float y = skill.value("y", (float)0);

            if (x > xMax || y > yMax) {
                logger::error("File {} did not specify a valid position for skill {}, skipping skill", filepath, i);
                continue;
            }

            // If the package is the internal package, skill lookup is not required
            // instead, the local IDs are stored directly
            if (package == "[[AttunementSkillbar]]") {
                if (!hasLocalID) {
                    logger::error("File {} did not specify a valid ID for internal icon {}, skipping skill", filepath, i);
                }
                
                uint32_t ID = skill.value("ID", (uint32_t)0);
                SkillTexture skillTexture = {};
                skillTexture.textureID = textureID;
                skillTexture.topLeft = ImVec2(x / xMax, y / yMax);
                skillTexture.bottomRight = ImVec2((x + 1) / xMax, (y + 1) / yMax);
                _skillTextureMap.emplace(ID, skillTexture);
                
                continue;
            }

            bool isGenericPotion = false;

            // For non-internal skills, determine the skill TESForm and ensure it is
            // a supported form type
            RE::TESForm *form = nullptr;
            if (hasEditorID) {
                std::string editorID = skill.value("editorID", std::string{});
                form = RE::TESForm::LookupByEditorID(editorID);

                if (!form || (form->formType != RE::FormType::Spell && form->formType != RE::FormType::Shout)) {
                    logger::error("File {} cannot find skill at {}, with editor ID {}", filepath, i, editorID);
                    continue;
                }
            }
            else {
                auto *dataHandler = RE::TESDataHandler::GetSingleton();
                if (!dataHandler) {
                    logger::error("File {} cannot find skill at {} because a system component failed to load", filepath, i);
                    continue;
                }

                uint32_t ID = skill.value("ID", (uint32_t)0);
                uint32_t baseID = ID;

                // For generic potion IDs, use the base potion id
                if (ID > 0x10000000) {
                    isGenericPotion = true;
                    baseID = ID - 0x10000000;
                }

                form = dataHandler->LookupForm(baseID, package);
            }

            if (
                !form || (
                    form->formType != RE::FormType::Spell &&
                    form->formType != RE::FormType::Shout &&
                    form->formType != RE::FormType::MagicEffect
                )
            ) {
                logger::error("File {} cannot find skill at {}, resolved to form {}, skipping skill", filepath, i, (void *)form);
                continue;
            }

            // Finally store the texture
            SkillTexture skillTexture = {};
            skillTexture.textureID = textureID;
            skillTexture.topLeft = ImVec2(x / xMax, y / yMax);
            skillTexture.bottomRight = ImVec2((x + 1) / xMax, (y + 1) / yMax);
            if (isGenericPotion) {
                _genericPotionTextureMap.emplace(form->GetFormID(), skillTexture);
            }
            else {
                _skillTextureMap.emplace(form->GetFormID(), skillTexture);
            }
        }

    }

    // MARK: Keybind textures
    
    void SkillHUD::LoadKeybindTexturesFromFile(const std::filesystem::path &path) {
        std::string filepath = path.string();

        std::ifstream inputStream(path);
        auto JSON = nlohmann::json::parse(inputStream);

        // Perform some basic validation on the JSON
        if (!JSON.contains("keybinds") || !JSON["keybinds"].is_array() || JSON["keybinds"].size() == 0) {
            logger::error("File {} does not contain any icons", filepath);
            return;
        }

        // First attempt to load the texture and retain a reference to it
        std::string texturePath = "Data\\SkillIcons\\Keybinds.png";

        ID3D11ShaderResourceView *textureID;
        int32_t width, height;
        bool loaded = Renderer::LoadTextureFromFile(
            texturePath.c_str(),
            &textureID,
            width,
            height
        );

        if (!loaded) {
            logger::error("File {} specified a texture \"{}\" that could not be loaded, skipping", filepath, texturePath);
            return;
        }

        float imageWidth = (float)width;
        float imageHeight = (float)height;

        // Enumerate all the skill icon references and create and store them in the map
        const auto &keybinds = JSON["keybinds"];
        const auto size = keybinds.size();
        for (size_t i = 0; i < size; i++) {
            const auto &keybind = keybinds[i];

            // Keybind must have key ID or key
            bool hasKeyID = true;
            bool hasKey = true;
            if (!keybind.contains("key") || !keybind["key"].is_string()) {
                hasKey = false;
            }
            if (!keybind.contains("keyID") || !keybind["keyID"].is_number_unsigned()) {
                hasKeyID = false;
            }

            if (!hasKeyID) {
                // TODO parse key id
                logger::error("File {} did not specify a valid ID for keybind {}, skipping keybind", filepath, i);
                continue;
            }

            // Keybind must have frame definition
            if (!keybind.contains("x") || !keybind["x"].is_number_unsigned()) {
                logger::error("File {} did not specify a valid position for keybind {}, skipping keybind", filepath, i);
                continue;
            }

            if (!keybind.contains("y") || !keybind["y"].is_number_unsigned()) {
                logger::error("File {} did not specify a valid position for keybind {}, skipping keybind", filepath, i);
                continue;
            }

            if (!keybind.contains("w") || !keybind["w"].is_number_unsigned()) {
                logger::error("File {} did not specify a valid position for keybind {}, skipping keybind", filepath, i);
                continue;
            }

            if (!keybind.contains("h") || !keybind["h"].is_number_unsigned()) {
                logger::error("File {} did not specify a valid position for keybind {}, skipping keybind", filepath, i);
                continue;
            }

            uint32_t keyID = keybind.value("keyID", (uint32_t)0);

            float x = keybind.value("x", (float)0);
            float y = keybind.value("y", (float)0);
            float w = keybind.value("w", (float)0);
            float h = keybind.value("h", (float)0);

            // Finally store the texture
            KeybindTexture keybindTexture = {};
            keybindTexture.textureID = textureID;
            keybindTexture.topLeft = ImVec2(x / imageWidth, y / imageHeight);
            keybindTexture.bottomRight = ImVec2((x + w) / imageWidth, (y + h) / imageHeight);
            keybindTexture.sizeRatio = w / h;
            _keybindTextureMap.emplace(keyID, keybindTexture);
        }

    }

    // MARK: Skill overrides

    /**
     * Loads the overrides defined in the specified JSON object.
     * @param JSON          The JSON.
     * @param filepath      The filepath used for error reporting.
     */
    void LoadOverridesFromJSON(const nlohmann::json &JSON, const std::string filepath) {
        // TODO: This should move to spell cast controller

        // Perform some basic validation on the JSON
        if (!JSON.contains("overrides") || !JSON["overrides"].is_array() || JSON["overrides"].size() == 0) {
            logger::error("File {} does not contain any overrides", filepath);
            return;
        }

        
        // Enumerate all the overrides and store them in the spell cast controller
        const auto &overrides = JSON["overrides"];
        const auto size = overrides.size();
        for (size_t i = 0; i < size; i++) {
            const auto &skill = overrides[i];

            // Skill must have an ID
            if (!skill.contains("ID") || !skill["ID"].is_number_unsigned()) {
                logger::error("File {} did not specify a valid ID for skill {}, skipping skill", filepath, i);
                continue;
            }

            // Skill must have package
            if (!skill.contains("package") || !skill["package"].is_string()) {
                logger::error("File {} did not specify a valid position for skill {}, skipping skill", filepath, i);
                continue;
            }

            // Ensure the ID refers to a spell
            RE::TESForm *form = nullptr;
            auto *dataHandler = RE::TESDataHandler::GetSingleton();
            if (!dataHandler) {
                logger::error("File {} cannot find skill at {} because a system component failed to load", filepath, i);
                continue;
            }

            uint32_t ID = skill.value("ID", (uint32_t)0);
            std::string package = skill.value("package", std::string{});
            form = dataHandler->LookupForm(ID, package);

            if (!form || form->formType != RE::FormType::Spell) {
                logger::error("File {} cannot find skill at {}, resolved to form {}, skipping skill", filepath, i, (void *)form);
                continue;
            }

            SpellOverrides spellOverrides {};
            if (skill.contains("isConcentration") && skill["isConcentration"].is_boolean()) {
                spellOverrides.overridesManuallyReleased = true;
                spellOverrides.manuallyReleased = skill.value("isConcentration", true);
            }

            SpellCastController::AddSpellOverride(form->GetFormID(), spellOverrides);
        }
    }

    // MARK: Theme

    void SkillHUD::LoadThemeFromFile(const std::filesystem::path &path) {
        std::string filepath = path.string();

        std::ifstream inputStream(path);
        auto JSON = nlohmann::json::parse(inputStream);

        LoadThemeValue(SkillSlotSizeBase)
        LoadThemeValue(SkillSlotSpacingBase)
        LoadThemeValue(SkillSlotKeybindHeightBase)
        LoadThemeValue(SkillSlotPressedScale)
        LoadThemeValue(SkillSlotHandIndicatorSizeBase)
        LoadThemeValue(AttunementSlotSizeDeselected)
        LoadThemeValue(AttunementSlotSpacingBase)
        LoadThemeValue(AttunementSlotKeybindHeightBase)
        LoadThemeValue(AttunementSlotPressedScale)
        LoadThemeValue(AttunementSlotSelectedScale)
        LoadThemeValue(SkillAttunementGap)
        LoadThemeValue(PotionSkillGap)
        LoadThemeValue(SkillSlotPressedAnimationDuration)
        LoadThemeValue(AttunementChangeAnimationDuration)

    }


    std::unordered_map<RE::FormID, SkillTexture> SkillHUD::_skillTextureMap;
    std::unordered_map<RE::FormID, SkillTexture> SkillHUD::_genericPotionTextureMap;
    std::unordered_map<RE::FormID, SkillTexture> SkillHUD::_internalTextureMap;
    std::unordered_map<RE::FormID, KeybindTexture> SkillHUD::_keybindTextureMap;
    SkillTexture SkillHUD::leftHandTexture;
    SkillTexture SkillHUD::rightHandTexture;
    SkillTexture SkillHUD::buttonTexture;
    SkillTexture SkillHUD::emptyButtonTexture;
    SkillHUDTheme SkillHUD::_theme;
    SkillHUD *SkillHUD::_sharedHUD = nullptr;

}