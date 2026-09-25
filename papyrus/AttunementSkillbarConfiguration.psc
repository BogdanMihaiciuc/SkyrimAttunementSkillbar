Scriptname AttunementSkillbarConfiguration Hidden

; =========================
; Attunements
; =========================

Int Function GetAttunementCount() Global Native
Function SetAttunementCount(Int count) Global Native

Int Function GetSkillCount() Global Native
Function SetSkillCount(Int count) Global Native

Int Function GetTextureIDForAttunement(Int index) Global Native
Function SetTextureIDForAttunement(Int index, Int textureID) Global Native

; =========================
; Generic Keybinds
; =========================

Int Function GetAssignmentModifierKeyID() Global Native
Function SetAssignmentModifierKeyID(Int keyID) Global Native

Int Function GetAssignRightHandKeyID() Global Native
Function SetAssignRightHandKeyID(Int keyID) Global Native

Int Function GetAssignLeftHandKeyID() Global Native
Function SetAssignLeftHandKeyID(Int keyID) Global Native

Int Function GetAssignDualCastModifierKeyID() Global Native
Function SetAssignDualCastModifierKeyID(Int keyID) Global Native

Int Function GetPotionKeyID() Global Native
Function SetPotionKeyID(Int keyID) Global Native

Int Function GetNextAttunementKeyID() Global Native
Function SetNextAttunementKeyID(Int keyID) Global Native

Int Function GetPreviousAttunementKeyID() Global Native
Function SetPreviousAttunementKeyID(Int keyID) Global Native

; =========================
; Behaviours
; =========================

Int Function GetSkillCastBehaviour() Global Native
Function SetSkillCastBehaviour(Int behaviour) Global Native

Int Function GetPotionSelectBehaviour() Global Native
Function SetPotionSelectBehaviour(Int behaviour) Global Native

Bool Function GetAutoCastsEquippedSpell() Global Native
Function SetAutoCastsEquippedSpell(Bool enabled) Global Native

Bool Function GetPrefersDualCast() Global Native
Function SetPrefersDualCast(Bool enabled) Global Native

; =========================
; Attunement Keybinds
; =========================

Int Function GetKeybindForAttunement(Int index) Global Native
Function SetKeybindForAttunement(Int index, Int keyID) Global Native

; =========================
; Skill Keybinds
; =========================

Int Function GetKeybindForSkill(Int index) Global Native
Function SetKeybindForSkill(Int index, Int keyID) Global Native

; =========================
; Show/Hide Configuration
; =========================

Bool Function GetShowsAlways() Global Native
Function SetShowsAlways(Bool enable) Global Native

Bool Function GetShowsInCombat() Global Native
Function SetShowsInCombat(Bool enable) Global Native

Bool Function GetShowsWeaponsDrawn() Global Native
Function SetShowsWeaponsDrawn(Bool enable) Global Native

; =========================
; Layout Configuration
; =========================

Float Function GetHUDScale() Global Native
Function SetHUDScale(Float scale) Global Native

Int Function GetHorizontalAnchorKind() Global Native
Function SetHorizontalAnchorKind(Int kind) Global Native

Int Function GetVerticalAnchorKind() Global Native
Function SetVerticalAnchorKind(Int kind) Global Native

Float Function GetHorizontalAnchorPoint() Global Native
Function SetHorizontalAnchorPoint(Float point) Global Native

Float Function GetVerticalAnchorPoint() Global Native
Function SetVerticalAnchorPoint(Float point) Global Native

Float Function GetMarginLeft() Global Native
Function SetMarginLeft(Float margin) Global Native

Float Function GetMarginRight() Global Native
Function SetMarginRight(Float margin) Global Native
