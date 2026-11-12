# Cutscenes on a flat screen that stays put (2026-11-12 evening, Fable, simulator)

Tefa: *"silent hill 2 cutscenes in 2d screen in front of me without depth"*, then *"flat screen should stay put on a
floating big screen"*, then (watching the simulator) *"right eye on the simulator was glitching and blinking, and the
game window showed this, also James head needs to be visible in cutscenes"*.

## What is built (`dev-archive/native/src/cutscreen.cpp`, installed as b007)

- Every 0.1 s the plugin asks the game's own `SHCharacterStatics::IsCharacterInCutscene(pawn)`. NOT the pack's wider
  cutscene state (main.lua also sets that while vaulting, climbing and pushing).
- Cutscene begins: `UI_FollowView=false`, `UI_Size=3.5`, `UI_Distance=3.0`, a recentre, then `VR_2DScreenMode=true`.
  Ends: `VR_2DScreenMode=false` and the three screen settings back to what they were (2 / 2 / follow in play).
  While flat, `UI_FollowView` is put back to false if the pack turns it on (its cutscene state locks the UI).
- **The settings are written from a short thread of our own, never from the engine tick.** Set from the tick, the
  switch killed the game the moment the opening cutscene began (access violation in the game under a UEVRBackend
  frame) `[verified-live 2026-11-12, n=1]`; the pack's `set_2D_mode` says the same ("do not execute in game thread").
- **The head in cutscenes** (`shadows.cpp`): since 2026-11-06 the head is hidden by an invisible material on its slots
  and nothing ever put it back, so James was headless in every cutscene. Each head slot's own material is now kept
  and put back while a cutscene plays, invisible again after; acted on at once, not at the 1 s check.
- Dev switch: a file `sh2vr_flat.txt` in the profile forces the flat screen on in play (for trying settings).

## Seen on the simulator

- New Game's opening cutscene, 3 min 46 s: flat screen on at its start, a surface 6.2 x 3.5 m, 3 m ahead at head
  height, the same for both eyes; it stays where it is when the head turns 25 degrees; back to full VR by itself at
  the end, the play screen's size back to 3.56 x 2 at 2 m `[verified-live 2026-11-12, n=2 full cutscenes]`.
- James has his head in the cutscene `[verified-live 2026-11-12, n=1, six stills + a wide shot]`.
- Forced flat in play at the armory save: on / off twice, walking, a head turn: clean, no crash `[verified-live, n=2]`.

## OPEN: the right eye is sometimes wrong while flat

- **Run 2** (the first full cutscene, Tefa watching): the simulator's own detector logged six `ASYMMETRIC_EYE_FLASH`
  incidents during the cutscene (frames 9418-11151) and one `ASYMMETRIC_BRIGHT_BREAKTHROUGH` at its end. Its frames
  (`dev-archive/recon/2026-11-12-cutscenes-flat-screen/right-eye-white-run2-incident-10229.png`): the left eye shows
  the cutscene, **the right eye a different, white, washed-out picture of a headless James in 11 frames of 12**, the
  right picture in 1. Tefa's still of the game window at the cutscene's end shows two views laid over each other
  with bands of smeared lines `[verified-live 2026-11-12, n=1 run]`.
- **Run 3** (the next full cutscene, the head fix in): no asymmetric incident in the whole cutscene; one captured
  moment with a band of smeared lines in the right eye only, just before a cut (`...-streak-run3-incident-12724.png`).
- Not reproduced by: forcing flat in play, walking, turning the head `[measured, n=1 each]`.
- UEVR logs once a second while flat: `FFakeStereoRenderingHook.cpp:2926 Something strange is going on, the vtable
  is already hooked, maybe previous frame was not rendered?`
- Leads `[hypothesis]`: the flat screen is drawn once per eye and the right eye's comes from its own picture, which
  in flat mode is not always drawn (the injector's native stereo, `VR_RenderingMethod=0`, with the game's DLSS and
  one DLSS instance per eye in this AFW build). To try, one at a time, over the opening cutscene with the detector's
  incident list as the measure: DLSS off while flat (`r.NGX.DLSS.Enable`), `VR_GhostingFix` off while flat,
  `VR_NativeStereoFix`, another `VR_RenderingMethod` while flat. What differed between run 2 and run 3 is not known
  (the head's materials changed; the switch may also land on a different moment of the frame each time).

## Driving notes

`sh2route.py` in the recon folder: launcher `play --sim --no-install`, 62 s, Enter (warning), Down + Enter (New
Game), Down x4 + Enter (Start Game), 32 s, Enter (press any button): the cutscene starts at once. The save folder was
copied away first and put back after (identical by hash); New Game did not replace the armory save.
