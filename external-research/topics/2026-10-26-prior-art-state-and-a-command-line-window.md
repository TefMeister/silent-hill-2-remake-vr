# Prior art, where it stands, and a command-line route for the window

**Date:** 2026-10-26 (`/gr`, FULL pass) · **Status:** 🆕 new · **For:** keeping the bundled parts' versions honest, and
the `[FLAT]` window row ("something rewrites it between runs").

## The parts this package builds on, checked on their public pages

`[reported 2026-10-26]`, from each project's GitHub page and release list:

| Project | Latest | Notes |
| --- | --- | --- |
| jbusfield, **SH2_UEVR** | v1.0.2, 2026-07-14/15 | An MIT licence file was added 2026-10-19; no code change since v1.0.2. The bundled copy is current. |
| praydog, **SH2R-UEVR** (`sh2r.dll`) | 0.3, 2024-11-27 | Last push 2024-11-30; still no licence file (the board already records the maintainer's decision). Unchanged since our first look. |
| PureDark, **UEVR AFW build** | UEVR_AFW_v1.0-beta.6, 2026-08-13 | Still the newest; last code push 2026-08-21 (UE 5.5.4 stereo fixes, memory-leak fix in its SDK). The tested build is current. |
| praydog, **UEVR** nightly | nightly 01143, 2026-08-30 | Nothing newer. |

No new Silent Hill 2 remake VR project turned up in two searches. A January 2026 "Silent Hill VR" announcement by
VRified Games is for the 1999 original, not this game `[reported]`. praydog's mod remains the other VR mod for
this game on our watch list (this project continues by the maintainer's choice), and it has not moved.

## The window keeps going back to fullscreen

Public material found nothing specific to this game `[reported 2026-10-26, two searches]`. Two general Unreal facts
that suggest a sturdier route than editing `GameUserSettings.ini`:

- Unreal's documented command-line switches include `-windowed` with `-ResX=` / `-ResY=` (Epic's command-line page;
  its own example is `-windowed -ResX=1080 -ResY=1920`) `[reported]`. Launched as `-windowed -ResX=1280 -ResY=720`, the
  window no longer depends on the ini surviving.
- Unreal forum threads describe shipped games resetting the ini's window mode at start, with the advice that a mode
  sticks only when the game applies it itself `[reported]`. A game that saves its own display choice elsewhere (this
  one keeps several settings in `PlayerProfile.sav`) could be what rewrites the ini `[hypothesis]`.

⚠️ Per the standing window rule, measure the SCREEN as well as the window after trying the switches: on another game a
width/height switch changed the whole desktop resolution while the window measured right.

## Sources

- jbusfield, SH2_UEVR — <https://github.com/jbusfield/SH2_UEVR>
- praydog, SH2R-UEVR — <https://github.com/praydog/SH2R-UEVR>
- PureDark, UEVR (AFW) — <https://github.com/PureDark/UEVR>
- praydog, UEVR-nightly — <https://github.com/praydog/UEVR-nightly>
- Outlook Respawn, VRified Games' Silent Hill VR (2026-01-05) —
  <https://respawn.outlookindia.com/gaming/gaming-news/vrified-games-reveals-silent-hill-vr-mod-with-motion-controls>
- Epic Games, "Command-line arguments in Unreal Engine" —
  <https://dev.epicgames.com/documentation/en-us/unreal-engine/command-line-arguments-in-unreal-engine>
- Unreal Engine AnswerHub, "Set fullscreen mode doesn't seem to work when a packaged game is launched" —
  <https://answers.unrealengine.com/questions/783467/view.html>
