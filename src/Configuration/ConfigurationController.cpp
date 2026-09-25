#include "ConfigurationController.h"
#include "../SkillHUD/SkillHUD.h"
#include "../SkillHUD/SkillHUDPresentationController.h"
#include "../SkillHUD/AttunementSlot.h"
#include "../SkillHUD/SkillSlot.h"
#include "../Input/InputEventController.h"
#include "../Input/SpellCastController.h"

namespace AttunementSkillbar {
    
    SkillHUDConfigurationAnchor ConfigurationController::AnchorForInt(int32_t value) {
        switch (value) {
            case SkillHUDConfigurationAnchor::Start:
                return SkillHUDConfigurationAnchor::Start;
            case SkillHUDConfigurationAnchor::Center:
                return SkillHUDConfigurationAnchor::Center;
            case SkillHUDConfigurationAnchor::End:
                return SkillHUDConfigurationAnchor::End;
            default:
                return SkillHUDConfigurationAnchor::Start;
        }
    }

    SkillCastBehaviour ConfigurationController::SkillCastBehaviourForInt(int32_t value) {
        switch (value) {
            case static_cast<int32_t>(SkillCastBehaviour::Equip):
                return SkillCastBehaviour::Equip;
            case static_cast<int32_t>(SkillCastBehaviour::Cast):
                return SkillCastBehaviour::Cast;
            case static_cast<int32_t>(SkillCastBehaviour::CastAndReequip):
                return SkillCastBehaviour::CastAndReequip;
            default:
                return SkillCastBehaviour::CastAndReequip;
        }
    }

    PotionSelectBehaviour ConfigurationController::PotionSelectBehaviourForInt(int32_t value) {
        switch (value) {
            case static_cast<int32_t>(PotionSelectBehaviour::Equip):
                return PotionSelectBehaviour::Equip;
            case static_cast<int32_t>(PotionSelectBehaviour::UseAndEquip):
                return PotionSelectBehaviour::UseAndEquip;
            default:
                return PotionSelectBehaviour::UseAndEquip;
        }
    }

    
    //MARK: Functional configuration

    int32_t ConfigurationController::GetAttunementCount(RE::StaticFunctionTag *) {
        return SkillHUD::SharedHUD()->GetAttunementCount();
    }

    void ConfigurationController::SetAttunementCount(RE::StaticFunctionTag *, int32_t count) {
        uint32_t newCount = count < 1 ? 1 : (uint32_t)count;
        SkillHUD::SharedHUD()->SetAttunementCount(newCount);
        InputEventController::SharedController()->SetAttunementCount(newCount);
    }

    int32_t ConfigurationController::GetSkillCount(RE::StaticFunctionTag *) {
        return SkillHUD::SharedHUD()->GetSkillCount();
    }

    void ConfigurationController::SetSkillCount(RE::StaticFunctionTag *, int32_t count) {
        uint32_t newCount = count < 1 ? 1 : (uint32_t)count;
        SkillHUD::SharedHUD()->SetSkillCount(newCount);
        InputEventController::SharedController()->SetSkillCount(newCount);
    }

    int32_t ConfigurationController::GetTextureIDForAttunement(RE::StaticFunctionTag *, int32_t index) {
        return SkillHUD::SharedHUD()->GetTextureIDForAttunement(index) - AttunementTextureOffset;
    }

    void ConfigurationController::SetTextureIDForAttunement(RE::StaticFunctionTag *, int32_t index, int32_t textureID) {
        SkillHUD::SharedHUD()->SetTextureIDForAttunement(index, textureID + AttunementTextureOffset);
    }


    int32_t ConfigurationController::GetSkillCastBehaviour(RE::StaticFunctionTag *) {
        return static_cast<int32_t>(SpellCastController::SharedController()->GetSkillCastBehaviour());
    }

    void ConfigurationController::SetSkillCastBehaviour(RE::StaticFunctionTag *, int32_t behaviour) {
        SpellCastController::SharedController()->SetSkillCastBehaviour(SkillCastBehaviourForInt(behaviour));
    }

    int32_t ConfigurationController::GetPotionSelectBehaviour(RE::StaticFunctionTag *) {
        return static_cast<int32_t>(SpellCastController::SharedController()->GetPotionSelectBehaviour());
    }

    void ConfigurationController::SetPotionSelectBehaviour(RE::StaticFunctionTag *, int32_t behaviour) {
        SpellCastController::SharedController()->SetPotionSelectBehaviour(PotionSelectBehaviourForInt(behaviour));
    }

    bool ConfigurationController::GetAutoCastsEquippedSpell(RE::StaticFunctionTag *) {
        return SpellCastController::SharedController()->GetAutoCastsEquippedSpell();
    }

    void ConfigurationController::SetAutoCastsEquippedSpell(RE::StaticFunctionTag *, bool enabled) {
        SpellCastController::SharedController()->SetAutoCastsEquippedSpell(enabled);
    }

    bool ConfigurationController::GetPrefersDualCast(RE::StaticFunctionTag *) {
        return SpellCastController::SharedController()->GetPrefersDualCast();
    }

    void ConfigurationController::SetPrefersDualCast(RE::StaticFunctionTag *, bool enabled) {
        SpellCastController::SharedController()->SetPrefersDualCast(enabled);
    }


    // MARK: Keybinds

    int32_t ConfigurationController::GetKeybindForAttunement(RE::StaticFunctionTag *, int32_t index) {
        return InputEventController::SharedController()->GetKeybindForAttunement(index).keyID;
    }

    void ConfigurationController::SetKeybindForAttunement(RE::StaticFunctionTag *, int32_t index, int32_t keyID) {
        if (keyID < 0) {
            keyID = 0;
        }

        InputEventController::SharedController()->SetKeybindForAttunement(index, { (uint32_t)keyID });
    }


    int32_t ConfigurationController::GetKeybindForSkill(RE::StaticFunctionTag *, int32_t index) {
        return InputEventController::SharedController()->GetKeybindForSkill(index).keyID;
    }

    void ConfigurationController::SetKeybindForSkill(RE::StaticFunctionTag *, int32_t index, int32_t keyID) {
        if (keyID < 0) {
            keyID = 0;
        }

        InputEventController::SharedController()->SetKeybindForSkill(index, { (uint32_t)keyID });
    }


    uint32_t ConfigurationController::GetAssignmentModifierKeyID(RE::StaticFunctionTag *) {
        return InputEventController::SharedController()->GetAssignmentModifierKeyID();
    }

    void ConfigurationController::SetAssignmentModifierKeyID(RE::StaticFunctionTag *, uint32_t keyID) {
        if (keyID < 0) {
            keyID = 0;
        }

        InputEventController::SharedController()->SetAssignmentModifierKeyID(keyID);
    }
    

    uint32_t ConfigurationController::GetAssignRightHandKeyID(RE::StaticFunctionTag *) {
        return InputEventController::SharedController()->GetAssignRightHandKeyID();
    }

    void ConfigurationController::SetAssignRightHandKeyID(RE::StaticFunctionTag *, uint32_t keyID) {
        if (keyID < 0) {
            keyID = 0;
        }

        InputEventController::SharedController()->SetAssignRightHandKeyID(keyID);
    }
    
    
    uint32_t ConfigurationController::GetAssignLeftHandKeyID(RE::StaticFunctionTag *) {
        return InputEventController::SharedController()->GetAssignLeftHandKeyID();
    }

    void ConfigurationController::SetAssignLeftHandKeyID(RE::StaticFunctionTag *, uint32_t keyID) {
        if (keyID < 0) {
            keyID = 0;
        }

        InputEventController::SharedController()->SetAssignLeftHandKeyID(keyID);
    }
    

    uint32_t ConfigurationController::GetAssignDualCastModifierKeyID(RE::StaticFunctionTag *) {
        return InputEventController::SharedController()->GetAssignDualCastModifierKeyID();
    }

    void ConfigurationController::SetAssignDualCastModifierKeyID(RE::StaticFunctionTag *, uint32_t keyID) {
        if (keyID < 0) {
            keyID = 0;
        }

        InputEventController::SharedController()->SetAssignDualCastModifierKeyID(keyID);
    }
    

    uint32_t ConfigurationController::GetPotionKeyID(RE::StaticFunctionTag *) {
        return InputEventController::SharedController()->GetPotionKeyID();
    }

    void ConfigurationController::SetPotionKeyID(RE::StaticFunctionTag *, uint32_t keyID) {
        if (keyID < 0) {
            keyID = 0;
        }

        InputEventController::SharedController()->SetPotionKeyID(keyID);
        auto configuration = SkillHUD::SharedHUD()->GetLayoutConfiguration();
        configuration.showsPotion = keyID != 0;
        SkillHUD::SharedHUD()->SetLayoutConfiguration(configuration);
    }

    
    uint32_t ConfigurationController::GetNextAttunementKeyID(RE::StaticFunctionTag *) {
        return InputEventController::SharedController()->GetNextAttunementKeyID();
    }

    void ConfigurationController::SetNextAttunementKeyID(RE::StaticFunctionTag *, int32_t keyID) {
        if (keyID < 0) {
            keyID = 0;
        }

        InputEventController::SharedController()->SetNextAttunementKeyID(keyID);
    }


    uint32_t ConfigurationController::GetPreviousAttunementKeyID(RE::StaticFunctionTag *) {
        return InputEventController::SharedController()->GetNextAttunementKeyID();
    }

    void ConfigurationController::SetPreviousAttunementKeyID(RE::StaticFunctionTag *, int32_t keyID) {
        if (keyID < 0) {
            keyID = 0;
        }

        InputEventController::SharedController()->SetPreviousAttunementKeyID(keyID);
    }

    
    // MARK: Show/hide configuration
    
    bool ConfigurationController::GetShowsAlways(RE::StaticFunctionTag *) {
        return SkillHUDPresentationController::SharedController()->GetShowsAlways();
    }

    void ConfigurationController::SetShowsAlways(RE::StaticFunctionTag *, bool enabled) {
        SkillHUDPresentationController::SharedController()->SetShowsAlways(enabled);
    }


    bool ConfigurationController::GetShowsInCombat(RE::StaticFunctionTag *) {
        return SkillHUDPresentationController::SharedController()->GetShowsInCombat();
    }

    void ConfigurationController::SetShowsInCombat(RE::StaticFunctionTag *, bool enabled) {
        SkillHUDPresentationController::SharedController()->SetShowsInCombat(enabled);
    }


    bool ConfigurationController::GetShowsWeaponsDrawn(RE::StaticFunctionTag *) {
        return SkillHUDPresentationController::SharedController()->GetShowsWeaponsDrawn();
    }

    void ConfigurationController::SetShowsWeaponsDrawn(RE::StaticFunctionTag *, bool enabled) {
        SkillHUDPresentationController::SharedController()->SetShowsWeaponsDrawn(enabled);
    }

    // MARK: Layout configuration
    
    float ConfigurationController::GetHUDScale(RE::StaticFunctionTag *) {
        return SkillHUD::SharedHUD()->GetLayoutConfiguration().sizeScale;
    }

    void ConfigurationController::SetHUDScale(RE::StaticFunctionTag *, float scale) {
        SkillHUDLayoutConfiguration config = SkillHUD::SharedHUD()->GetLayoutConfiguration();
        config.sizeScale = std::clamp(scale, 0.5f, 5.0f);
        SkillHUD::SharedHUD()->SetLayoutConfiguration(config);
    }


    int32_t ConfigurationController::GetHorizontalAnchorKind(RE::StaticFunctionTag *) {
        return static_cast<int32_t>(SkillHUD::SharedHUD()->GetLayoutConfiguration().horizontalAnchor);
    }

    void ConfigurationController::SetHorizontalAnchorKind(RE::StaticFunctionTag *, int32_t kind) {
        SkillHUDLayoutConfiguration config = SkillHUD::SharedHUD()->GetLayoutConfiguration();
        config.horizontalAnchor = AnchorForInt(kind);
        SkillHUD::SharedHUD()->SetLayoutConfiguration(config);
    }


    int32_t ConfigurationController::GetVerticalAnchorKind(RE::StaticFunctionTag *) {
        return static_cast<int32_t>(SkillHUD::SharedHUD()->GetLayoutConfiguration().verticalAnchor);
    }

    void ConfigurationController::SetVerticalAnchorKind(RE::StaticFunctionTag *, int32_t kind) {
        SkillHUDLayoutConfiguration config = SkillHUD::SharedHUD()->GetLayoutConfiguration();
        config.verticalAnchor = AnchorForInt(kind);
        SkillHUD::SharedHUD()->SetLayoutConfiguration(config);
    }


    float ConfigurationController::GetHorizontalAnchorPoint(RE::StaticFunctionTag *) {
        return SkillHUD::SharedHUD()->GetLayoutConfiguration().anchorPoint.x;
    }

    void ConfigurationController::SetHorizontalAnchorPoint(RE::StaticFunctionTag *, float point) {
        SkillHUDLayoutConfiguration config = SkillHUD::SharedHUD()->GetLayoutConfiguration();
        config.anchorPoint.x = point;
        SkillHUD::SharedHUD()->SetLayoutConfiguration(config);
    }


    float ConfigurationController::GetVerticalAnchorPoint(RE::StaticFunctionTag *) {
        return SkillHUD::SharedHUD()->GetLayoutConfiguration().anchorPoint.y;
    }

    void ConfigurationController::SetVerticalAnchorPoint(RE::StaticFunctionTag *, float point) {
        SkillHUDLayoutConfiguration config = SkillHUD::SharedHUD()->GetLayoutConfiguration();
        config.anchorPoint.y = point;
        SkillHUD::SharedHUD()->SetLayoutConfiguration(config);
    }


    float ConfigurationController::GetMarginLeft(RE::StaticFunctionTag *) {
        return SkillHUD::SharedHUD()->GetLayoutConfiguration().marginLeft;
    }

    void ConfigurationController::SetMarginLeft(RE::StaticFunctionTag *, float value) {
        SkillHUDLayoutConfiguration config = SkillHUD::SharedHUD()->GetLayoutConfiguration();
        config.marginLeft = value;
        SkillHUD::SharedHUD()->SetLayoutConfiguration(config);
    }


    float ConfigurationController::GetMarginRight(RE::StaticFunctionTag *) {
        return SkillHUD::SharedHUD()->GetLayoutConfiguration().marginRight;
    }

    void ConfigurationController::SetMarginRight(RE::StaticFunctionTag *, float value) {
        SkillHUDLayoutConfiguration config = SkillHUD::SharedHUD()->GetLayoutConfiguration();
        config.marginLeft = value;
        SkillHUD::SharedHUD()->SetLayoutConfiguration(config);
    }

    // MARK: Script registration

    bool ConfigurationController::InitializeScript(RE::BSScript::IVirtualMachine *vm) {
        vm->RegisterFunction("GetAttunementCount", "AttunementSkillbarConfiguration", GetAttunementCount);
        vm->RegisterFunction("SetAttunementCount", "AttunementSkillbarConfiguration", SetAttunementCount);

        vm->RegisterFunction("GetSkillCount", "AttunementSkillbarConfiguration", GetSkillCount);
        vm->RegisterFunction("SetSkillCount", "AttunementSkillbarConfiguration", SetSkillCount);

        vm->RegisterFunction("GetTextureIDForAttunement", "AttunementSkillbarConfiguration", GetTextureIDForAttunement);
        vm->RegisterFunction("SetTextureIDForAttunement", "AttunementSkillbarConfiguration", SetTextureIDForAttunement);

        vm->RegisterFunction("GetSkillCastBehaviour", "AttunementSkillbarConfiguration", GetSkillCastBehaviour);
        vm->RegisterFunction("SetSkillCastBehaviour", "AttunementSkillbarConfiguration", SetSkillCastBehaviour);

        vm->RegisterFunction("GetPotionSelectBehaviour", "AttunementSkillbarConfiguration", GetPotionSelectBehaviour);
        vm->RegisterFunction("SetPotionSelectBehaviour", "AttunementSkillbarConfiguration", SetPotionSelectBehaviour);

        vm->RegisterFunction("GetAutoCastsEquippedSpell", "AttunementSkillbarConfiguration", GetAutoCastsEquippedSpell);
        vm->RegisterFunction("SetAutoCastsEquippedSpell", "AttunementSkillbarConfiguration", SetAutoCastsEquippedSpell);

        vm->RegisterFunction("GetPrefersDualCast", "AttunementSkillbarConfiguration", GetPrefersDualCast);
        vm->RegisterFunction("SetPrefersDualCast", "AttunementSkillbarConfiguration", SetPrefersDualCast);


        vm->RegisterFunction("GetKeybindForAttunement", "AttunementSkillbarConfiguration", GetKeybindForAttunement);
        vm->RegisterFunction("SetKeybindForAttunement", "AttunementSkillbarConfiguration", SetKeybindForAttunement);

        vm->RegisterFunction("GetKeybindForSkill", "AttunementSkillbarConfiguration", GetKeybindForSkill);
        vm->RegisterFunction("SetKeybindForSkill", "AttunementSkillbarConfiguration", SetKeybindForSkill);


        vm->RegisterFunction("GetAssignmentModifierKeyID", "AttunementSkillbarConfiguration", GetAssignmentModifierKeyID);
        vm->RegisterFunction("SetAssignmentModifierKeyID", "AttunementSkillbarConfiguration", SetAssignmentModifierKeyID);

        vm->RegisterFunction("GetAssignRightHandKeyID", "AttunementSkillbarConfiguration", GetAssignRightHandKeyID);
        vm->RegisterFunction("SetAssignRightHandKeyID", "AttunementSkillbarConfiguration", SetAssignRightHandKeyID);

        vm->RegisterFunction("GetAssignLeftHandKeyID", "AttunementSkillbarConfiguration", GetAssignLeftHandKeyID);
        vm->RegisterFunction("SetAssignLeftHandKeyID", "AttunementSkillbarConfiguration", SetAssignLeftHandKeyID);

        vm->RegisterFunction("GetAssignDualCastModifierKeyID", "AttunementSkillbarConfiguration", GetAssignDualCastModifierKeyID);
        vm->RegisterFunction("SetAssignDualCastModifierKeyID", "AttunementSkillbarConfiguration", SetAssignDualCastModifierKeyID);

        vm->RegisterFunction("GetPotionKeyID", "AttunementSkillbarConfiguration", GetPotionKeyID);
        vm->RegisterFunction("SetPotionKeyID", "AttunementSkillbarConfiguration", SetPotionKeyID);


        vm->RegisterFunction("GetNextAttunementKeyID", "AttunementSkillbarConfiguration", GetNextAttunementKeyID);
        vm->RegisterFunction("SetNextAttunementKeyID", "AttunementSkillbarConfiguration", SetNextAttunementKeyID);

        vm->RegisterFunction("GetPreviousAttunementKeyID", "AttunementSkillbarConfiguration", GetPreviousAttunementKeyID);
        vm->RegisterFunction("SetPreviousAttunementKeyID", "AttunementSkillbarConfiguration", SetPreviousAttunementKeyID);


        vm->RegisterFunction("GetShowsAlways", "AttunementSkillbarConfiguration", GetShowsAlways);
        vm->RegisterFunction("SetShowsAlways", "AttunementSkillbarConfiguration", SetShowsAlways);

        vm->RegisterFunction("GetShowsInCombat", "AttunementSkillbarConfiguration", GetShowsInCombat);
        vm->RegisterFunction("SetShowsInCombat", "AttunementSkillbarConfiguration", SetShowsInCombat);

        vm->RegisterFunction("GetShowsWeaponsDrawn", "AttunementSkillbarConfiguration", GetShowsWeaponsDrawn);
        vm->RegisterFunction("SetShowsWeaponsDrawn", "AttunementSkillbarConfiguration", SetShowsWeaponsDrawn);


        vm->RegisterFunction("GetHUDScale", "AttunementSkillbarConfiguration", GetHUDScale);
        vm->RegisterFunction("SetHUDScale", "AttunementSkillbarConfiguration", SetHUDScale);

        vm->RegisterFunction("GetHorizontalAnchorKind", "AttunementSkillbarConfiguration", GetHorizontalAnchorKind);
        vm->RegisterFunction("SetHorizontalAnchorKind", "AttunementSkillbarConfiguration", SetHorizontalAnchorKind);

        vm->RegisterFunction("GetVerticalAnchorKind", "AttunementSkillbarConfiguration", GetVerticalAnchorKind);
        vm->RegisterFunction("SetVerticalAnchorKind", "AttunementSkillbarConfiguration", SetVerticalAnchorKind);



        vm->RegisterFunction("GetHorizontalAnchorPoint", "AttunementSkillbarConfiguration", GetHorizontalAnchorPoint);
        vm->RegisterFunction("SetHorizontalAnchorPoint", "AttunementSkillbarConfiguration", SetHorizontalAnchorPoint);

        vm->RegisterFunction("GetVerticalAnchorPoint", "AttunementSkillbarConfiguration", GetVerticalAnchorPoint);
        vm->RegisterFunction("SetVerticalAnchorPoint", "AttunementSkillbarConfiguration", SetVerticalAnchorPoint);

        vm->RegisterFunction("GetMarginLeft", "AttunementSkillbarConfiguration", GetMarginLeft);
        vm->RegisterFunction("SetMarginLeft", "AttunementSkillbarConfiguration", SetMarginLeft);

        vm->RegisterFunction("GetMarginRight", "AttunementSkillbarConfiguration", GetMarginRight);
        vm->RegisterFunction("SetMarginRight", "AttunementSkillbarConfiguration", SetMarginRight);

        return true;
    }

}