#include "AttunementSlot.h"
#include "SkillHUD.h"
#include "SkillSlot.h"
#include "imgui_internal.h"

namespace AttunementSkillbar {

    AttunementSlot::~AttunementSlot() {
        // When an attunement slot is deleted, it also deletes all its
        // skill slots
        if (!_skillSlots) {
            return;
        }

        for (uint32_t i = 0; i < _skillSlotCount; i++) {
            delete _skillSlots[i];
        }

        delete _skillSlots;
    }

    void AttunementSlot::SetKeybindTexture(KeybindTexture texture) {
        _keybindTexture = texture;
    }

    void AttunementSlot::SetConfiguration(
        SkillHUD *HUD,
        AttunementSlotConfiguration config,
        SkillSlotConfiguration *slotConfig
    ) {
        _configuration = config;
        _skillHUD = HUD;
        _skillSlotCount = HUD->_configuration.skillSlotCount;

        _skillSlots = new SkillSlot *[_skillSlotCount];

        // Create the actual skill slots
        for (uint32_t i = 0; i < _skillSlotCount; i++) {
            SkillSlot *slot = CreateSkillSlot();
            SkillSlotConfiguration slotConfiguration;
            if (slotConfig) {
                slotConfiguration = slotConfig[i];
            }
            slotConfiguration.index = i;
            slot->SetConfiguration(slotConfiguration);
            _skillSlots[i] = slot;
        }

        _texture = SkillHUD::GetTextureForAttunement(config.textureID);
    }

    void AttunementSlot::Resize(uint32_t count) {

        uint32_t oldCount = _skillSlotCount;
        _skillSlotCount = count;

        auto oldSkillSlots = _skillSlots;
        _skillSlots = new SkillSlot *[count];

        // Copy over as many old skill slots as will fit in the new configuration
        for (size_t i = 0; i < count && i < oldCount; i++) {
            _skillSlots[i] = oldSkillSlots[i];
        }

        // Delete all skill slots that no longer fit
        for (size_t i = count; i < oldCount; i++) {
            delete oldSkillSlots[i];
        }
        delete oldSkillSlots;

        // Create new skill slots for the new slots
        for (size_t i = oldCount; i < count; i++) {
            auto *newSlot = CreateSkillSlot();

            SkillSlotConfiguration slotConfiguration;
            slotConfiguration.index = (uint32_t)i;
            newSlot->SetConfiguration(slotConfiguration);

            _skillSlots[i] = newSlot;
        }

    }

    void AttunementSlot::Render(float, float scaleY, float translateY, float opacity) {
        // If there is only a single attunement defined, it does not need to be rendered
        if (_skillHUD->GetAttunementCount() < 2) {
            return;
        }

        const auto &theme = SkillHUD::Theme();

        auto size = theme.AttunementSlotSizeDeselected * _scale;
        auto height = size * scaleY;

        ImVec2 location = _position;
        location.y += translateY;
        location.y += (size - height) / 2.0f;

        auto bottomRight = ImVec2 { location.x + size, location.y + height };
        auto color = 0x00FFFFFF | ((uint32_t)(opacity * 255) << 24);

        auto drawList = ImGui::GetWindowDrawList();

        // Draw the attunement icon texture
        drawList->AddImage(
            _texture.textureID,
            location, bottomRight,
            _texture.topLeft, _texture.bottomRight,
            color
        );

        // Then add the button border on top of it
        auto borderTexture = SkillHUD::GetSkillSlotBorderTexture();
        drawList->AddImage(
            borderTexture.textureID,
            location, bottomRight,
            borderTexture.topLeft, borderTexture.bottomRight,
            color
        );

        // Finally add the keybind if defined
        if (!_keybindTexture.textureID) {
            return;
        }

        float keybindHeight = theme.AttunementSlotKeybindHeightBase * _scale;
        float keybindWidth = keybindHeight * _keybindTexture.sizeRatio;
        keybindHeight *= scaleY;

        ImVec2 keybindTopLeft = {
            location.x + size / 2.0f - keybindWidth / 2.0f,
            location.y + height - keybindHeight / 2.0f
        };

        ImVec2 keybindBottomRight = {
            keybindTopLeft.x + keybindWidth,
            keybindTopLeft.y + keybindHeight
        };

        drawList->AddImage(
            _keybindTexture.textureID,
            keybindTopLeft, keybindBottomRight,
            _keybindTexture.topLeft, _keybindTexture.bottomRight,
            color
        );
    }

    SkillSlot *AttunementSlot::CreateSkillSlot() {
        return new SkillSlot();
    }

}