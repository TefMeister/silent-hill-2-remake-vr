# 2026-09-26: both community profiles, read for one question

**Session:** home PC (PC1), static only, nothing launched. Claude on Opus, Tefa present.
**Question Tefa set:** we build our own UEVR profile, on the **UEVR nightly AFW** build, from zero. Before
that, look at how the two existing profiles do what they do, learn from them, copy nothing. The idea that
makes ours different: **use the game's own first-person hand animations** where the game has them (pushing
a cupboard, pulling a lever, crawling through a gap) instead of driving every arm movement from the
controllers. First we have to find out whether that is possible at all.

## What was looked at

Tefa's local folder `D:\SH2 to learn from and not copy\` (never committed, never redistributed):

| Archive | What it is | Size |
| --- | --- | --- |
| `SHProto-Win64-Shipping (1).zip` | jbusfield's profile (IK arms and body) | 92 files, 59 Lua files, **49,718 Lua lines**, `sh2r.dll`, `uevr_utils.dll` |
| `SHProto Win64 Shipping 421 3.3 2026-09-23T15-50Z alf5wbRh4.zip` | CharlotteLiu's mod, built on jbusfield's | 195 files, 87 Lua files, **94,159 Lua lines**, `SH2.dll`, `bhpatics_bridge_UE.dll`, `TrueGear_bridge_UE.dll`, `sim_input_bridge_UE.dll` |
| `UEVR-nightly_AFW_v1.0-beta.6.zip` | PureDark's AFW build of UEVR | `UEVRBackend.dll`, `PDAFWPlugin.dll`, `LuaVR.dll`, injector, OpenXR/OpenVR loaders |

Counted with `find`/`wc -l` on the unpacked copies `[measured 2026-09-26]`.

⭐ **The jbusfield archive is the same version the dev PC studied on 2026-09-19**: identical Lua line count
(49,718). So dossier §8e already covers it, and nothing there needs redoing `[measured 2026-09-26]`.

## What CharlotteLiu adds on top

Not a small patch: it nearly doubles the Lua. From the file list and a first read `[inferred-static
2026-09-26]`:

- **Replaces** jbusfield's `sh2r.dll` and `uevr_utils.dll` with its own `SH2.dll`, and adds bridges for
  bHaptics and TrueGear vests and a simulated-input bridge.
- New scripts `UEVR_00` to `UEVR_11` (globals, keyboard/mouse input, bindings, haptics, actions,
  **gestures**, environment, an "Immersion Enhancer"), plus `hands.lua`, `heal.lua`, reload tools.
- **`unlock_door.lua`, 5,487 lines, comments in Chinese.** This is the interesting one for us. It turns
  the world's interactive objects into things **your own hands move**: door latch bars you grab and slide,
  keys in doors, sliding doors, drawers, a **pushable wardrobe** with 40x40 cm grab faces on its front,
  a **cart** that follows your hand 1:1 while locked to the floor, and **levers** pressed down by hand with
  the game's own lever sound played on the lever's sound component. Dated notes inside run 2026-08-30 to
  2026-09-02.
- Its animation list adds three entries to jbusfield's: `Handgun_Reload` (played from an animation
  sequence in the default slot, which is how the manual reload is shown), the raw handgun reload
  sequence, and `James_PH_Grab` (Pyramid Head's grab).

## ⭐ What each does with the game's own animations

Both profiles keep an **animation list**: 34 entries in jbusfield's, 37 in CharlotteLiu's. Each entry says,
while that animation plays, whether the VR hands, each arm, James's body, his arms, arm bones, motion
sickness compensation and controller input stay on. The `Any` entry is the default for everything
unlisted `[inferred-static 2026-09-26, from montage.lua:166-175]`.

**Every listed animation is combat, a struggle, a face animation or the save point** — melee swings,
chainsaw attacks, weapon reloads, grabs by enemies, death faces, the safe-code animation. **Not one is an
interaction animation**: no push, no pull, no lever, no crawl, no squeeze through a gap
`[measured 2026-09-26]`.

For pushing, jbusfield's `main.lua:946-960` does one thing: while `Movement.PushableComponent` exists, it
treats the moment **as a cutscene** and switches roomscale movement off, with the comment *"or the game
will prevent the task from ever being completed."* The push animation itself is not shown in first person.

So the two existing answers are:

| Profile | What happens when the game wants to push, pull or open something |
| --- | --- |
| jbusfield | the moment is treated like a cutscene; the game's animation is not used as first-person arms |
| CharlotteLiu | the game's animation is **replaced**: your real hands grab and move the object, hand-built per object type |

**Nobody has tried Tefa's idea**: let the game's own animation play and show it as your arms, then hand
the arms back to the controllers smoothly. That makes it genuinely new, and it means neither profile can
tell us whether it works. Only the game can.

## Why it might work, and the one thing to find out first

Dossier §8e found that both profiles make the VR arms from a copy of **James's own third-person body**
(there is no separate first-person mesh), and that when an animation from their list plays, the profile
**copies the game's pose onto the arms** instead of running IK. That copy path is exactly the mechanism
Tefa's idea needs, already working for fights and struggles `[inferred-static 2026-09-19]`.

What is unknown is whether the **interaction** animations look right from James's eyes. They were made for
a camera behind him: hands may be off to the side, under the view, or clipping through the coat. That is
a question about pictures, not code, and it is cheap to answer.

**The first test, when the game is next running:** with plain UEVR (AFW), a camera on James's head bone and
his full body left visible, walk up to a pushable wardrobe, a lever and a crawl gap, trigger each, and
look. If the arms land somewhere believable, the idea is alive. If they fly off-screen, it tells us which
animations need a correction and which are hopeless. Gate: `[FLAT]` for the first look on a monitor,
`[VR USER]` for whether it feels right.

## Tefa's decision, recorded

**We build on the UEVR nightly AFW build** (Tefa, 2026-09-26). This supersedes dossier §8h (2026-09-19),
where AFW was to stay optional and never a dependency. What that means for releases (users fetch the AFW
build themselves; we redistribute nothing of it) is noted in the dossier, not decided here.

## What is NOT established

- Nothing here was seen running. Semantics of the animation-list switches come from reading the code, not
  from watching them.
- `unlock_door.lua` was skimmed through its header notes, not read line by line. Object names in it
  (`SHPushable`, `SHSlidingDoor`, `InteractiveDrawer_Base_ABP_C`, `SHAk_Lever`) are theirs, from their
  config and comments, not yet checked against the game.
- The AFW build's contents were listed, not examined.
- Nothing from either profile is copied into this repo. Names of the game's own objects and animations are
  facts about the game, recorded so they can be checked live.

## Addendum, same night: one profile for every rendering method

Tefa, 2026-09-26: *"if we can build on both native stereo and AFW at the same time, that would be great!"*
They remembered the AFW build's in-game menu offering AFW, native stereo and AFR.

**Confirmed from the build itself** (strings in `UEVRBackend.dll` of `UEVR-nightly_AFW_v1.0-beta.6`)
`[inferred-static 2026-09-26]`: a **Rendering Method** setting with **Native Stereo**, **Synchronized
Sequential**, **Alternating/AFR** and **Alternate Frame Warping**, plus a "Native Stereo Fix". The build
also says *"Using DX11, AFW only supports DX12, fallback to AFR"*; this game runs on DX12, so AFW is
available here.

**So building on the AFW build costs nothing in choice: native stereo is in the same build.** The design
rule that follows: **our profile must work in every rendering method, and never assume one.** The trap
is the sequential modes, where one game frame draws only one eye. Anything we do once per frame (IK, the
game-animation handover, body yaw) has to be done once per *game tick*, not once per *eye*, or the two
eyes disagree. jbusfield's own framework has a comment about exactly this for another game
(`input.lua`: body yaw "needs to be calculated for both eyes") `[inferred-static 2026-09-26]`.
Every live test therefore runs at least twice: once in Native Stereo, once in AFW.
