# The full VR button and body layout: every button and every holster, in one design

Order: 6005
From: the ideas repo, `games/silent-hill-2-remake.md` (<https://github.com/TefMeister/mod-ideas/blob/main/games/silent-hill-2-remake.md>), copied 2026-10-25

`[raw]` · `[looks doable]` for the buttons, `[hard]` for the syringe, the drink and the shoulder flashlight beam — *the button part was checked against the mod's own code on 2026-10-25 (three button layers: the game, the pack's Lua, our C++); the game's own button list was not read yet*

**What Tefa said (2026-10-25, in the headset):** firing without the grip works; the hand reload works but the buttons
fight, so the buttons come first. The wanted layout:

| control | what it does |
| --- | --- |
| left stick click | start / stop running (a toggle) |
| B | eject the pistol's magazine (and nothing else: today one press also dodged) |
| A | interact and vault; with an item held in the hand (some quest items), flip it |
| X | back, in examine, inventory and notes |
| Y | straight to the inventory |
| left stick direction + right stick down | dodge that way |
| right stick up | quick turn; a setting, OFF by default |
| left grip + left trigger | rack the slide / work the pump |
| left grip | grab things in the world; the right grip too when that hand is empty |
| right grip | hold a weapon; let go and it goes back to its holster |

| where | left grip | right grip |
| --- | --- | --- |
| left shoulder ("left top holster") | flashlight: take it in the hand / put it back on the shoulder, the beam from where it sits; with it in the hand, no two-handed weapons | rifle |
| left hip | map; while it is held, left and right grip switch floor / area | the equipped melee weapon (plank, pipe, chainsaw) |
| right hip | syringe: appears in the left hand, bring it to the right wrist | handgun |
| right shoulder | health drink: bring it to the mouth | shotgun |

**Same for every RE game we work on, RE8 included:** the right grip holds the weapon and letting go holsters it, and
the stick dodge (left stick direction + right stick down).

**⚠️ This changes the 2026-09-28 settled six spots** (above): the map moves from the right grip to the LEFT grip at the
left hip, the melee weapon takes the right grip there, the flashlight moves from above the head to the left shoulder,
and the ammo pouch is not in this list. Tefa to confirm which wins before it is built.

**What it'd take:** (1) one button layer, ours, that reads the real controller and decides what the game sees, with the
pack's Lua button rules (right stick down presses B, the stick press turned into LB for sprint) folded into it, so no
press ever reaches two things; it needs the game's own button for inventory, back and quick turn read first, and a
"menu or gameplay" check. (2) the holster spots are the same family as the hip pouches in Visceral RE2 and RE8.
(3) the syringe, the drink and the flashlight beam from the shoulder need new hand items and the game's own use calls:
the hard part.

Verbatim record: [`inbox/2026-10-25b-sh2-full-vr-button-and-body-layout.md`](../inbox/2026-10-25b-sh2-full-vr-button-and-body-layout.md)
