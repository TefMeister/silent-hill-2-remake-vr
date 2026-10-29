# A poser tool that works in the headset

Order: 6003
From: the ideas repo, `games/silent-hill-2-remake.md` (<https://github.com/TefMeister/mod-ideas/blob/main/games/silent-hill-2-remake.md>), copied 2026-10-25

`[raw]` · `[looks doable]` — *checked against the mod's own files on 2026-10-24, not tried in the game yet*

> "can you figure out a poser tool that i can use in vr for Silent Hill 2?"

Verbatim record: [`inbox/2026-10-24-sh2-vr-poser-tool.md`](../inbox/2026-10-24-sh2-vr-poser-tool.md)

For RE2 the poser (EMV Engine) only works on the flat screen. Silent Hill 2 already carries one that works in the
headset: jbusfield's **Hands Creation Tool**, part of the SH2_UEVR scripts we bundle. It sits in the UEVR menu, which
shows in the headset and is worked with the controller pointer, and its own instructions say to use it wearing the
headset: pick a gun, pick a finger, turn each joint with sliders, test it "holding weapon", and it saves into
`hands_parameters.json` (grip poses already exist there for pistol, shotgun, rifle, pipe, board, chainsaw,
flashlight, paper). It is switched off by one commented-out line in `main.lua` (`hands.enableConfigurationTool()`).

**What it'd take:** switch that line on for test runs. ⚠️ Our C++ arms (`ik.cpp`) take the fingers from the game's own
arm animation, not from `hands_parameters.json` `[inferred-static 2026-10-24]`, so poses made in the tool show on the
Lua hands; to show them on our C++ arms, the arms have to read the saved poses (a no-game job). A "grab a finger and
bend it" poser of our own is possible later, but sliders come first because they already exist.

Chosen by Tefa: 2026-10-29
