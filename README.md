# Attunement Skillbar

A Skyrim SKSE mod built using [CommonLibSSE NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) that adds an on-screen skillbar to which spells, powers, lesser powers, shouts, potions and scrolls can be assigned. Because Skyrim has a lot of spells, the standard favorite keybinds are insufficient to be able to use all spells a player would want to in realtime during combat or exploration without pausing and opening the menu. Most keybind mods also have the same limitations in terms of the available keys that can be reached comfortably during gameplay.

## Compatibility

I have only tested this on AE 1.6.1179 (GOG) since this is the only version I have access to.

## Basic concept

Attunement skillbar combines some ideas from other games, primarily the elementalist class in Guild Wars 2 to provide multiple customize skillbars that can be switched between using a second set of "attunment" buttons. Each attunement is associated with one skillbar and each can be assigned a keybind to switch to its skillbar, or a generic next/previous attunement can be used.

The player can configure, via MCM, however many attunements and skill slots per attunement they want.

It also takes inspiration from Elder Scrolls Online, by adding a "consumable button" which also has its own space for assigning multiple consumables (potions and scrolls) from the inventory. A single press of the consumable keybind uses that consumable, whereas pressing and holding that key allows to switch the active consumable to another one that has been assigned to a slot. The consumable slots reuse the skill keybinds and slots.

## Additional QOL

Attunement skillbar includes some additional QOL features to make mage gameplay more enjoyable.

By default, clicking on any skill causes the player character to perform that skill. For spells, a single keypress is sufficient - the mod will automate casting the spell and then swap back to whatever was previously equipped. This can be changed to have the mod just act as a simple equipment swapper.

Additionally, if the player character has a spell equipped, automatic casting can also be turned on for that spell. Pressing the attack key will cause the player character to cast that spell without the player needing to hold to charge. Combined with the automatic switching, this makes it possible to have a "main attack spell" equipped and used without need to assign it to the skillbar.

The mod also queues spells. If this feature is enabled, pressing a spell key while the player is already in the middle of an automated spell cast, that spell will be queued and performed after the current spell has finished casting.

Finally, if the main spell is dual wielded or if any spell is added to the skillbar as a dual cast and the player has the relevant perk, the mod will perform a dual cast without needing to press both attack skills at once. If a dual cast is not possible but a single cast is, the mod will fall back to performing the single cast.

## Limitations and bugs

I mostly built this mod for my own playthrough and it worked well enough for me, but there are some bugs and limitations that I am aware of:
 - When automatic casting is turned on, performing two different casts with each hand won't work
 - Sometimes the player can cast spells via the skillbar in situations where the game would otherwise disable player input (e.g. during some cutscenes where you can't normally control your character)
 - Sometimes automated casting will fizzle out and the character will stop responding to skillbar keybinds. The mod will timeout of this state on its own in a few seconds, but stowing weapons, dodging, jumping or attacking with a martial weapon will cause the mod to reset instantly in these situations
 - Sometimes automated casting will become unresponsive when quickly switching between a martial weapon and spells in the main hand because of the drawing animation. The workaround is the same as the previous issue

There are likely more bugs that I haven't encountered in my own play through.

## Building

This is based on [
HelloWorld-using-CommonLibSSE-NG](https://github.com/SkyrimDev/HelloWorld-using-CommonLibSSE-NG). The build steps are detailed there.

## Credits

I have taken inspiration or portions of code from the following repositories:
https://github.com/adamhynek/instant_equip_vr - equipment delay skipping idea
https://github.com/D7ry/wheeler/ - integration with skyui menus, logging, input event blocking, rendering, unique item id creation
https://github.com/max-su-2019/MaxsuDetectionMeter - UI drawing with imGui
https://github.com/pWn3d1337/Skyrim_SpellHotbar2/ - overall inspiration