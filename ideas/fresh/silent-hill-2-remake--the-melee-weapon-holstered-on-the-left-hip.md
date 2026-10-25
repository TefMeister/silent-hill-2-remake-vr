# The melee weapon holstered on the left hip

Order: 6004
From: the ideas repo, `games/silent-hill-2-remake.md` (<https://github.com/TefMeister/mod-ideas/blob/main/games/silent-hill-2-remake.md>), copied 2026-10-25

`[raw]` · `[looks doable]` — *not checked against the game: the pack's holster spots exist, the melee weapon's own equip path has not been read*

**What Tefa said (2026-10-24):** the melee weapon (pipe, plank, the chainsaw) should sit holstered in the left hip spot
and be drawn from there by hand, instead of appearing in the hand the moment the trigger is pressed. Same session: the
reload system follows the RE8 recipe (left grip puts the hand on the part, LT works it), being built in C++.

**What it'd take:** a holster zone on the left hip bound to the headset in tracking space (the standing rule: never
on the body's joints), the melee weapon's mesh shown parked there while a gun is in hand, and a grab (left grip in the
zone) that calls the game's own equip (`SHWeaponManageCmbSubcomp.RequestEquipWeapon`, proven live for the pistol) and
takes the hand with it; the trigger-press equip is then blocked while the holster is in use. Fits beside the six-spots
idea above. ⚠️ Not checked: whether the melee weapon can be shown without being equipped, and which input the game
uses for the quick melee swap.

Verbatim record: [`inbox/2026-10-24-sh2-melee-holstered-on-the-left-hip.md`](../inbox/2026-10-24-sh2-melee-holstered-on-the-left-hip.md)
