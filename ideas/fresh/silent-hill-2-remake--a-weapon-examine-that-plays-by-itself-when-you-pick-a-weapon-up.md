# A weapon examine that plays by itself when you pick a weapon up

Order: 6008
From: the ideas repo, `games/silent-hill-2-remake.md` (<https://github.com/TefMeister/mod-ideas/blob/main/games/silent-hill-2-remake.md>), copied 2026-11-08

`[raw]` · `[not judged]` — ⚠️ *nothing checked against the game yet*

**What Tefa said (2026-11-08):** *"weapon examine automatic play, where game takes control over hands, i will record
the examine hands and gun clip in game that triggers when the weapon is picked up. All weapons must be empty at fist,
and have some ammo in the same room with them."*

Reads as: the first time a weapon is picked up, the hands are taken off the controllers and play a short clip Tefa
records in the game (the hands turning the gun over, checking it), then given back. Every weapon starts empty, and
some ammo for it lies in the same room, so the first reload happens by hand right there.

**What it'd take:** three parts. (1) The clip: Tefa records the hands and the gun in the game; we play it back on the
hands when the pickup happens, and the hand-over in and out must be smooth, never a snap (the same hard part as the
all-games idea "Let the game's own animations take over the hands", which this is the first real case of). (2) Spotting
the pickup: the moment a weapon enters the inventory, read through UEVR. (3) Empty weapons and ammo in the same room:
the starting rounds set to zero and an ammo pickup placed beside each weapon, which means changing where items lie in
each level, the costly part.

Verbatim record: [`inbox/2026-11-08-sh2-weapon-examine-and-per-hand-smoothing.md`](../inbox/2026-11-08-sh2-weapon-examine-and-per-hand-smoothing.md)
