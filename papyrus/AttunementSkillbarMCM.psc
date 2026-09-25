Scriptname AttunementSkillbarMCM extends SKI_ConfigBase

Int AnchorStart  = 1
Int AnchorCenter = 2
Int AnchorEnd    = 3

Float ScaleExtraSmall = 0.5
Float ScaleSmall = 0.75
Float ScaleMedium = 1.0
Float ScaleLarge = 1.5
Float ScaleExtraLarge = 2.0

String[] AnchorLabels
String[] SkillCastBehaviourLabels
String[] PotionSelectBehaviourLabels
String[] TextureIDLabels
String[] HUDScaleLabels

;=========================
; Option ID Variables
;=========================

; Basic counts
Int optionAttunementCount
Int optionSkillCount

; Keybinds
Int optionAssignRightHand
Int optionAssignLeftHand
Int optionAssignmentModifier
Int optionDualCastModifier
Int optionPotionKey
Int optionNextAttunementKey
Int optionPreviousAttunementKey

; Behaviour
Int optionSkillCastBehaviour
Int optionPotionSelectBehaviour
Int optionAutoCastsEquippedSepll
Int optionPrefersDualCast

; Attunement keybinds
Int[] optionAttunementKey

; Skill keybinds
Int[] optionSkillKey

; Attunement icons
Int[] optionAttunementIcon

; Show/hide

Int optionShowsAlways
Int optionShowsInCombat
Int optionShowsWeaponsDrawn

; Appearance
Int optionHUDScale
Int optionHUDScaleSlider
Int optionHorizontalAnchor
Int optionVerticalAnchor

; Advanced positioning
Int optionAnchorLeftOffset
Int optionAnchorTopOffset
Int optionMarginLeft
Int optionMarginRight

String rejectMessageInt = "Please enter a whole, positive number."
String rejectMessageFloat = "Please enter a number."

Bool Function IsValidIntString(String s)
    If s == ""
        return false
    EndIf

    Int i = 0
    Int len = StringUtil.GetLength(s)

    ; optional sign
    String c0 = StringUtil.GetNthChar(s, 0)
    If c0 == "+" || c0 == "-"
        If len == 1
            return false
        EndIf
        i = 1
    EndIf

    While i < len
        If !StringUtil.IsDigit(StringUtil.GetNthChar(s, i))
            return false
        EndIf
        i += 1
    EndWhile

    return true
EndFunction

Bool Function IsValidFloatString(String s)
    If s == ""
        return false
    EndIf

    Int i = 0
    Int len = StringUtil.GetLength(s)
    Bool sawDot = false
    Bool sawDigit = false

    ; optional sign
    String c0 = StringUtil.GetNthChar(s, 0)
    If c0 == "+" || c0 == "-"
        If len == 1
            return false
        EndIf
        i = 1
    EndIf

    While i < len
        String c = StringUtil.GetNthChar(s, i)
        If StringUtil.IsDigit(c)
            sawDigit = true
        ElseIf c == "."
            If sawDot
                return false
            EndIf
            sawDot = true
        Else
            return false
        EndIf
        i += 1
    EndWhile

    ; require at least one digit
    return sawDigit
EndFunction

Function RejectInput(Int option, String message, String revertValue)
    debug.notification("Please provide a numeric value.")
    ; Put the UI back to the saved value
    SetInputOptionValue(option, revertValue)
EndFunction


Int Function GetScaleLabelIndex(Float scale)
    Float delta = 0.01

    If Math.Abs(scale - ScaleExtraSmall) < delta
        return 0
    ElseIf Math.Abs(scale - ScaleSmall) < delta
        return 1
    ElseIf Math.Abs(scale - ScaleMedium) < delta
        return 2
    ElseIf Math.Abs(scale - ScaleLarge) < delta
        return 3
    ElseIf Math.Abs(scale - ScaleExtraLarge) < delta
        return 4
    EndIf

    return 5
EndFunction

Int Function KeybindWithUnassignment(Int keybind)
    Int result = keybind
    If result == 0
        result = -1
    Endif
    Return result
EndFunction

Event OnConfigInit()
    ModName = "Attunement Skillbar"

	Pages = new string[3]
	Pages[0] = "General"
	Pages[1] = "Skills and Attunements"
	Pages[2] = "Appearance"


    AnchorLabels = new String[3]
    AnchorLabels[0] = "Start"
    AnchorLabels[1] = "Center"
    AnchorLabels[2] = "End"

    SkillCastBehaviourLabels = new String[3]
    SkillCastBehaviourLabels[0] = "Equip"
    SkillCastBehaviourLabels[1] = "Cast"
    SkillCastBehaviourLabels[2] = "Cast and Re-equip"

    PotionSelectBehaviourLabels = new String[2]
    PotionSelectBehaviourLabels[0] = "Equip"
    PotionSelectBehaviourLabels[1] = "Use and Equip"


    TextureIDLabels = new String[20]
    ; Basic magic schools
    TextureIDLabels[0] = "Alteration" ; + 18
    TextureIDLabels[1] = "Conjuration"
    TextureIDLabels[2] = "Destruction"
    TextureIDLabels[3] = "Illusion"
    TextureIDLabels[4] = "Restoration"
    TextureIDLabels[5] = "Shout"
    TextureIDLabels[6] = "Spell"
    TextureIDLabels[7] = "Power"

    ; Generic spells
    TextureIDLabels[8] = "Fire Spell"
    TextureIDLabels[9] = "Ice Spell"
    TextureIDLabels[10] = "Lightning Spell"
    TextureIDLabels[11] = "Destructive Energy Spell"
    TextureIDLabels[12] = "Defense Spell"
    TextureIDLabels[13] = "Healing Spell"
    TextureIDLabels[14] = "Buff Spell"
    TextureIDLabels[15] = "Mental Spell"
    TextureIDLabels[16] = "Mental Energy Spell"
    TextureIDLabels[17] = "Shadow Spell"
    TextureIDLabels[18] = "Summoning Spell"
    TextureIDLabels[19] = "Necromancy Spell"

    
    HUDScaleLabels = new String[6]
    HUDScaleLabels[0] = "Extra Small"
    HUDScaleLabels[1] = "Small"
    HUDScaleLabels[2] = "Default"
    HUDScaleLabels[3] = "Large"
    HUDScaleLabels[4] = "Extra Large"
    HUDScaleLabels[5] = "Custom"

EndEvent

Event OnPageReset(String page)
	If (page == "General")
		RenderConfiguration()
	ElseIf (page == "Skills and Attunements")
        RenderSkillKeybinds()
	ElseIf (page == "Appearance")
        RenderAppearance()
	EndIf
EndEvent

Function RenderConfiguration()
    SetCursorFillMode(TOP_TO_BOTTOM)
    SetCursorPosition(0)

    Int attunementCount = AttunementSkillbarConfiguration.GetAttunementCount()
    Int skillCount = AttunementSkillbarConfiguration.GetSkillCount()

    ; Basic count settings
    AddHeaderOption("Button counts")
    optionAttunementCount = AddInputOption("Number of attunements", attunementCount as String)
    optionSkillCount = AddInputOption("Number of skills", skillCount as String)

    ; Assignment keybinds
    AddHeaderOption("Assignment keybinds")
    optionAssignRightHand = AddKeyMapOption("Right key", KeybindWithUnassignment(AttunementSkillbarConfiguration.GetAssignRightHandKeyID()), OPTION_FLAG_WITH_UNMAP)
    optionAssignLeftHand = AddKeyMapOption("Left key", KeybindWithUnassignment(AttunementSkillbarConfiguration.GetAssignLeftHandKeyID()), OPTION_FLAG_WITH_UNMAP)
    optionAssignmentModifier = AddKeyMapOption("Modifier (optional)", KeybindWithUnassignment(AttunementSkillbarConfiguration.GetAssignmentModifierKeyID()), OPTION_FLAG_WITH_UNMAP)
    optionDualCastModifier = AddKeyMapOption("Dual cast modifier", KeybindWithUnassignment(AttunementSkillbarConfiguration.GetAssignDualCastModifierKeyID()), OPTION_FLAG_WITH_UNMAP)
    optionPotionKey = AddKeyMapOption("Potion key", KeybindWithUnassignment(AttunementSkillbarConfiguration.GetPotionKeyID()), OPTION_FLAG_WITH_UNMAP)
    optionNextAttunementKey = AddKeyMapOption("Next attunement key", KeybindWithUnassignment(AttunementSkillbarConfiguration.GetNextAttunementKeyID()), OPTION_FLAG_WITH_UNMAP)
    optionPreviousAttunementKey = AddKeyMapOption("Previous attunement key", KeybindWithUnassignment(AttunementSkillbarConfiguration.GetPreviousAttunementKeyID()), OPTION_FLAG_WITH_UNMAP)

    AddHeaderOption("Behaviours")
    optionSkillCastBehaviour = AddMenuOption("Skill behaviour", SkillCastBehaviourLabels[AttunementSkillbarConfiguration.GetSkillCastBehaviour() - 1])
    optionPotionSelectBehaviour = AddMenuOption("Potion select behaviour", PotionSelectBehaviourLabels[AttunementSkillbarConfiguration.GetPotionSelectBehaviour() - 1])
    optionAutoCastsEquippedSepll = AddToggleOption("Auto-cast equipped spell", AttunementSkillbarConfiguration.GetAutoCastsEquippedSpell())
    optionPrefersDualCast = AddToggleOption("Attempt dual auto-casting", AttunementSkillbarConfiguration.GetPrefersDualCast())

    SetCursorPosition(1)
    ; Add some basic instructions on the right
    AddTextOption("To assign skills to the skillbar highlight the", "", OPTION_FLAG_DISABLED)
    AddTextOption("skill in the spell menu and press the appropriate", "", OPTION_FLAG_DISABLED)
    AddTextOption("left/right hand keybind defined here with the", "", OPTION_FLAG_DISABLED)
    AddTextOption("modifier if defined.", "", OPTION_FLAG_DISABLED)
    AddTextOption("To enable dual casting for that spell, also hold", "", OPTION_FLAG_DISABLED)
    AddTextOption("the dual casting modifier.", "", OPTION_FLAG_DISABLED)
    AddEmptyOption()
    AddTextOption("For potions, the right key assigns a generic potion", "", OPTION_FLAG_DISABLED)
    AddTextOption("where the largest potion of that type is used when", "", OPTION_FLAG_DISABLED)
    AddTextOption("consumed, while left key assigns the highlighted", "", OPTION_FLAG_DISABLED)
    AddTextOption("potion specifically", "", OPTION_FLAG_DISABLED)
    
EndFunction

Function RenderSkillKeybinds()
    Int attunementCount = AttunementSkillbarConfiguration.GetAttunementCount()
    Int skillCount = AttunementSkillbarConfiguration.GetSkillCount()

    SetCursorFillMode(TOP_TO_BOTTOM)
    SetCursorPosition(0)

    ; Attunement and skill keybinds
    AddHeaderOption("Attunement keybinds")
    optionAttunementKey = Utility.createIntArray(attunementCount, 0)
    Int i = 0
    While i < attunementCount
        Int keybind = AttunementSkillbarConfiguration.GetKeybindForAttunement(i)
        If keybind == 0
            keybind = -1
        Endif

        optionAttunementKey[i] = AddKeyMapOption("Attunement " + (i + 1), keybind, OPTION_FLAG_WITH_UNMAP)
        i += 1
    EndWhile

    AddEmptyOption()
    AddHeaderOption("Skill keybinds")
    optionSkillKey = Utility.createIntArray(skillCount, 0)
    i = 0
    While i < skillCount
        Int keybind = AttunementSkillbarConfiguration.GetKeybindForSkill(i)
        If keybind == 0
            keybind = -1
        Endif

        optionSkillKey[i] = AddKeyMapOption("Skill " + (i + 1), keybind, OPTION_FLAG_WITH_UNMAP)
        i += 1
    EndWhile

    ; Move to the right for the attunement icon configuration
    SetCursorPosition(1)
    
    AddHeaderOption("Attunement icons")
    optionAttunementIcon = Utility.createIntArray(attunementCount, 0)
    i = 0
    While i < attunementCount
        Int textureID = AttunementSkillbarConfiguration.GetTextureIDForAttunement(i)
        If textureID > 19
            textureID = 19
        EndIf

        optionAttunementIcon[i] = AddMenuOption("Attunement icon " + (i + 1), TextureIDLabels[textureID])
        i += 1
    EndWhile
    
EndFunction

Function RenderAppearance()
    SetCursorFillMode(TOP_TO_BOTTOM)
    SetCursorPosition(0)

    ; Basic appearance
    Float HUDScale = AttunementSkillbarConfiguration.GetHUDScale()
    Int HUDScaleIndex = GetScaleLabelIndex(HUDScale)

    AddHeaderOption("Size and position")
    optionHUDScale = AddMenuOption("Size", HUDScaleLabels[HUDScaleIndex])
    If HUDScaleIndex == 5
        optionHUDScaleSlider = AddSliderOption("Size scale", HUDScale, "{2}")
    EndIf
    optionHorizontalAnchor = AddMenuOption("Horizontal", AnchorLabels[AttunementSkillbarConfiguration.GetHorizontalAnchorKind() - 1])
    optionVerticalAnchor = AddMenuOption("Vertical", AnchorLabels[AttunementSkillbarConfiguration.GetVerticalAnchorKind() - 1])

    ; Exact positioning
    AddHeaderOption("Advanced positioning")
    optionAnchorLeftOffset = AddInputOption("Left offset", AttunementSkillbarConfiguration.GetHorizontalAnchorPoint() as String)
    optionAnchorTopOffset = AddInputOption("Top offset", AttunementSkillbarConfiguration.GetVerticalAnchorPoint() as String)
    optionMarginLeft = AddInputOption("Left margin", AttunementSkillbarConfiguration.GetMarginLeft() as String)
    optionMarginRight = AddInputOption("Right margin", AttunementSkillbarConfiguration.GetMarginRight() as String)

    SetCursorPosition(1)
    AddHeaderOption("Show/hide")
    optionShowsAlways = AddToggleOption("Always show", AttunementSkillbarConfiguration.GetShowsAlways())

    If !AttunementSkillbarConfiguration.GetShowsAlways()
        optionShowsInCombat = AddToggleOption("Show in combat", AttunementSkillbarConfiguration.GetShowsInCombat())
        optionShowsWeaponsDrawn = AddToggleOption("Show while weapons drawn", AttunementSkillbarConfiguration.GetShowsWeaponsDrawn())
    EndIf
EndFunction


;=========================
; Event Handlers
;=========================

Event OnOptionSelect(Int option)
    If option == optionAutoCastsEquippedSepll
        Bool value = !AttunementSkillbarConfiguration.GetAutoCastsEquippedSpell()
        AttunementSkillbarConfiguration.SetAutoCastsEquippedSpell(value)
        SetToggleOptionValue(option, value)

    ElseIf option == optionPrefersDualCast
        Bool value = !AttunementSkillbarConfiguration.GetPrefersDualCast()
        AttunementSkillbarConfiguration.SetPrefersDualCast(value)
        SetToggleOptionValue(option, value)

    ElseIf option == optionShowsAlways
        Bool value = !AttunementSkillbarConfiguration.GetShowsAlways()
        AttunementSkillbarConfiguration.SetShowsAlways(value)
        SetToggleOptionValue(option, value)
        ForcePageReset()

    ElseIf option == optionShowsInCombat
        Bool value = !AttunementSkillbarConfiguration.GetShowsInCombat()
        AttunementSkillbarConfiguration.SetShowsInCombat(value)
        SetToggleOptionValue(option, value)

    ElseIf option == optionShowsWeaponsDrawn
        Bool value = !AttunementSkillbarConfiguration.GetShowsWeaponsDrawn()
        AttunementSkillbarConfiguration.SetShowsWeaponsDrawn(value)
        SetToggleOptionValue(option, value)
    EndIf

EndEvent

Event OnOptionKeyMapChange(Int option, Int keyCode, String conflictControl, String conflictName)
    ; Generic assignment keybinds
    If option == optionAssignRightHand
        AttunementSkillbarConfiguration.SetAssignRightHandKeyID(keyCode)
        SetKeyMapOptionValue(option, keyCode)

    ElseIf option == optionAssignLeftHand
        AttunementSkillbarConfiguration.SetAssignLeftHandKeyID(keyCode)
        SetKeyMapOptionValue(option, keyCode)

    ElseIf option == optionAssignmentModifier
        AttunementSkillbarConfiguration.SetAssignmentModifierKeyID(keyCode)
        SetKeyMapOptionValue(option, keyCode)

    ElseIf option == optionDualCastModifier
        AttunementSkillbarConfiguration.SetAssignDualCastModifierKeyID(keyCode)
        SetKeyMapOptionValue(option, keyCode)

    ElseIf option == optionPotionKey
        AttunementSkillbarConfiguration.SetPotionKeyID(keyCode)
        SetKeyMapOptionValue(option, keyCode)

    ElseIf option == optionNextAttunementKey
        AttunementSkillbarConfiguration.SetNextAttunementKeyID(keyCode)
        SetKeyMapOptionValue(option, keyCode)

    ElseIf option == optionPreviousAttunementKey
        AttunementSkillbarConfiguration.SetPreviousAttunementKeyID(keyCode)
        SetKeyMapOptionValue(option, keyCode)

    Else
        ; Attunement keybinds range
        Int attCount = AttunementSkillbarConfiguration.GetAttunementCount()
        Int i = 0
        While i < attCount
            If optionAttunementKey[i] == option
                AttunementSkillbarConfiguration.SetKeybindForAttunement(i, keyCode)
                SetKeyMapOptionValue(option, keyCode)
                Return
            EndIf
            i += 1
        EndWhile

        ; Skill keybinds range
        Int skCount = AttunementSkillbarConfiguration.GetSkillCount()
        i = 0
        While i < skCount
            If optionSkillKey[i] == option
                AttunementSkillbarConfiguration.SetKeybindForSkill(i, keyCode)
                SetKeyMapOptionValue(option, keyCode)
                Return
            EndIf
            i += 1
        EndWhile
    EndIf
EndEvent


Event OnOptionMenuOpen(Int option)
    ; HUD scale preset menu
    If option == optionHUDScale
        Int curIndex = GetScaleLabelIndex(AttunementSkillbarConfiguration.GetHUDScale())
        SetMenuDialogOptions(HUDScaleLabels)
        SetMenuDialogStartIndex(curIndex)
        SetMenuDialogDefaultIndex(2) ; "Default"
        return
    EndIf

    ; Anchor menus
    If option == optionHorizontalAnchor
        Int startIdx = AttunementSkillbarConfiguration.GetHorizontalAnchorKind() - 1
        SetMenuDialogOptions(AnchorLabels)
        SetMenuDialogStartIndex(startIdx)
        SetMenuDialogDefaultIndex(0)
        return
    ElseIf option == optionVerticalAnchor
        Int vIdx = AttunementSkillbarConfiguration.GetVerticalAnchorKind() - 1
        SetMenuDialogOptions(AnchorLabels)
        SetMenuDialogStartIndex(vIdx)
        SetMenuDialogDefaultIndex(0)
        return
    ElseIf option == optionSkillCastBehaviour
        Int cIdx = AttunementSkillbarConfiguration.GetSkillCastBehaviour() - 1
        SetMenuDialogOptions(SkillCastBehaviourLabels)
        SetMenuDialogStartIndex(cIdx)
        SetMenuDialogDefaultIndex(0)
        return
    ElseIf option == optionPotionSelectBehaviour
        Int pIdx = AttunementSkillbarConfiguration.GetPotionSelectBehaviour() - 1
        SetMenuDialogOptions(PotionSelectBehaviourLabels)
        SetMenuDialogStartIndex(pIdx)
        SetMenuDialogDefaultIndex(0)
        return
    EndIf

    ; Attunement icon menus (dynamic range)
    Int attCount = AttunementSkillbarConfiguration.GetAttunementCount()
    Int i = 0
    While i < attCount
        If optionAttunementIcon[i] == option
            Int tex = AttunementSkillbarConfiguration.GetTextureIDForAttunement(i)
            If tex < 0
                tex = 0
            ElseIf tex > 19
                tex = 19
            EndIf

            SetMenuDialogOptions(TextureIDLabels)
            SetMenuDialogStartIndex(tex)
            SetMenuDialogDefaultIndex(0)
            Return
        EndIf
        i += 1
    EndWhile
EndEvent


Event OnOptionMenuAccept(Int option, Int index)
    ; HUD scale preset menu
    If option == optionHUDScale
        If index == 0
            AttunementSkillbarConfiguration.SetHUDScale(ScaleExtraSmall)
        ElseIf index == 1
            AttunementSkillbarConfiguration.SetHUDScale(ScaleSmall)
        ElseIf index == 2
            AttunementSkillbarConfiguration.SetHUDScale(ScaleMedium)
        ElseIf index == 3
            AttunementSkillbarConfiguration.SetHUDScale(ScaleLarge)
        ElseIf index == 4
            AttunementSkillbarConfiguration.SetHUDScale(ScaleExtraLarge)
        Else
            ; Custom: keep current scale, but ensure slider is visible
            ; (force rebuild so the slider appears if it wasn't)
        EndIf

        SetMenuOptionValue(optionHUDScale, HUDScaleLabels[index])

        ; If switching between custom/non-custom, rebuild the page so slider appears/disappears.
        ForcePageReset()
        return
    EndIf

    ; Anchor menus
    If option == optionHorizontalAnchor
        AttunementSkillbarConfiguration.SetHorizontalAnchorKind(index + 1)
        SetMenuOptionValue(option, AnchorLabels[index])
        return
    ElseIf option == optionVerticalAnchor
        AttunementSkillbarConfiguration.SetVerticalAnchorKind(index + 1)
        SetMenuOptionValue(option, AnchorLabels[index])
        return
    ElseIf option == optionSkillCastBehaviour
        AttunementSkillbarConfiguration.SetSkillCastBehaviour(index + 1)
        SetMenuOptionValue(option, SkillCastBehaviourLabels[index])
        return
    ElseIf option == optionPotionSelectBehaviour
        AttunementSkillbarConfiguration.SetPotionSelectBehaviour(index + 1)
        SetMenuOptionValue(option, PotionSelectBehaviourLabels[index])
        return
    EndIf

    ; Attunement icons (dynamic range)
    Int attCount = AttunementSkillbarConfiguration.GetAttunementCount()
    Int i = 0
    While i < attCount
        If optionAttunementIcon[i] == option
            AttunementSkillbarConfiguration.SetTextureIDForAttunement(i, index)
            SetMenuOptionValue(option, TextureIDLabels[index])
            Return
        EndIf
        i += 1
    EndWhile
EndEvent


Event OnOptionSliderOpen(Int option)
    If option == optionHUDScaleSlider
        Float cur = AttunementSkillbarConfiguration.GetHUDScale()
        SetSliderDialogStartValue(cur)
        SetSliderDialogDefaultValue(ScaleMedium)
        SetSliderDialogRange(0.25, 3.0)
        SetSliderDialogInterval(0.01)
        return
    EndIf
EndEvent


Event OnOptionSliderAccept(Int option, Float value)
    If option == optionHUDScaleSlider
        AttunementSkillbarConfiguration.SetHUDScale(value)
        SetSliderOptionValue(option, value, "{2}")

        ; Ensure the menu label shows "Custom"
        SetMenuOptionValue(optionHUDScale, HUDScaleLabels[5])
        return
    EndIf
EndEvent


Event OnOptionInputOpen(Int option)
    If option == optionAttunementCount
        SetInputDialogStartText(AttunementSkillbarConfiguration.GetAttunementCount() as String)
        return
    ElseIf option == optionSkillCount
        SetInputDialogStartText(AttunementSkillbarConfiguration.GetSkillCount() as String)
        return
    ElseIf option == optionAnchorLeftOffset
        SetInputDialogStartText(AttunementSkillbarConfiguration.GetHorizontalAnchorPoint() as String)
        return
    ElseIf option == optionAnchorTopOffset
        SetInputDialogStartText(AttunementSkillbarConfiguration.GetVerticalAnchorPoint() as String)
        return
    ElseIf option == optionMarginLeft
        SetInputDialogStartText(AttunementSkillbarConfiguration.GetMarginLeft() as String)
        return
    ElseIf option == optionMarginRight
        SetInputDialogStartText(AttunementSkillbarConfiguration.GetMarginRight() as String)
        return
    EndIf
EndEvent


Event OnOptionInputAccept(Int option, String text)

    ; Counts (ints)
    If option == optionAttunementCount
        If !IsValidIntString(text)
            RejectInput(option, rejectMessageInt, AttunementSkillbarConfiguration.GetAttunementCount() as String)
            return
        EndIf

        Int newCount = text as Int
        AttunementSkillbarConfiguration.SetAttunementCount(newCount)
        ForcePageReset()
        return

    ElseIf option == optionSkillCount
        If !IsValidIntString(text)
            RejectInput(option, rejectMessageInt, AttunementSkillbarConfiguration.GetSkillCount() as String)
            return
        EndIf

        Int newSkillCount = text as Int
        AttunementSkillbarConfiguration.SetSkillCount(newSkillCount)
        ForcePageReset()
        return
    EndIf

    ; Advanced positioning (floats)
    If option == optionAnchorLeftOffset
        If !IsValidFloatString(text)
            RejectInput(option, rejectMessageFloat, AttunementSkillbarConfiguration.GetHorizontalAnchorPoint() as String)
            return
        EndIf

        Float v = text as Float
        AttunementSkillbarConfiguration.SetHorizontalAnchorPoint(v)
        SetInputOptionValue(option, v as String)
        return
    ElseIf option == optionAnchorTopOffset
        If !IsValidFloatString(text)
            RejectInput(option, rejectMessageFloat, AttunementSkillbarConfiguration.GetVerticalAnchorPoint() as String)
            return
        EndIf

        Float v2 = text as Float
        AttunementSkillbarConfiguration.SetVerticalAnchorPoint(v2)
        SetInputOptionValue(option, v2 as String)
        return

    ElseIf option == optionMarginLeft
        If !IsValidFloatString(text)
            RejectInput(option, rejectMessageFloat, AttunementSkillbarConfiguration.GetMarginLeft() as String)
            return
        EndIf

        Float m1 = text as Float
        AttunementSkillbarConfiguration.SetMarginLeft(m1)
        SetInputOptionValue(option, m1 as String)
        return

    ElseIf option == optionMarginRight
        If !IsValidFloatString(text)
            RejectInput(option, rejectMessageFloat, AttunementSkillbarConfiguration.GetMarginRight() as String)
            return
        EndIf

        Float m2 = text as Float
        AttunementSkillbarConfiguration.SetMarginRight(m2)
        SetInputOptionValue(option, m2 as String)
        return
    EndIf
EndEvent

