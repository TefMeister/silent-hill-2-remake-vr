# Research index

**Last `/gr` pass: 2026-11-11 — FULL.** The quit abort inside the OpenXR Simulator (code 7) matches its own preview thread, a static `std::thread` joined only in `xrDestroyInstance` since v1.6.0 (unfixed on master; the fork's v1.5.1 has no such thread); the UEVR crash at `+0x1503df` still has no upstream change or public match; master also lifts the 4096 picture cap for Direct3D, unreleased.

_Previous: **Last `/gr` pass: 2026-11-07 (estate sweep) — CHECK-IN.** For Gold Digger's crash dumps: UEVR nightly (01143, 08-30), the AFW release (beta.6, 08-13) and both SH2 packs have not changed, so the quit crash of 11-06 on did not come with a UEVR update; beta.6 ships as two different dlls (nightly- and joeyhodge-based), so name ours before reading the offset; no public report matches either crash. One topic, pointer in `engine-research/inbox/`._

_Previous: **Last `/gr` pass: 2026-11-01 (estate sweep) — CHECK-IN.** One search on the frame rate halving of 2026-10-31: nothing public ties the 21 Sacraments patch (bigbullet3, Nexus; aimed at the ray-tracing workload) to a 30 fps cap; the game itself locks cutscenes to 30 `[reported]`. Nothing else new._

_Previous: **Last `/gr` pass: 2026-10-26 (estate sweep) — FULL.** Folder created this pass. The button layer's open question is answered from UEVR's source: C++ plugins edit the pad before the Lua scripts, so the pack's own pad writes land on top of ours; the game's default buttons, the shadow flags, and the state of every bundled part also checked._

Status tags: 🆕 new · 👀 being looked at · ✅ used / confirmed · ❌ dead end.

| Date | Topic | Status | Summary |
| --- | --- | --- | --- |
| 2026-11-11 | [The quit abort inside the OpenXR Simulator is its own preview thread](topics/2026-11-11-the-quit-abort-is-the-simulators-preview-thread.md) | 🆕 | Since v1.6.0 a static `std::thread` is joined only in `xrDestroyInstance`; a quit without it aborts (code 7, CRT linked into the dll). Unfixed on master; fork v1.5.1 lacks the thread. UEVR `+0x1503df`: nothing upstream again. Master lifts the 4096 cap for D3D, unreleased. Pointer in `engine-research/inbox/`. |
| 2026-11-07 | [The quit crash and the UEVR crash of 11-05/06: nothing upstream changed](topics/2026-11-07-the-quit-crash-and-the-uevr-crash-nothing-upstream-changed.md) | 🆕 | No UEVR, AFW or SH2-pack change since August/October; AFW beta.6 comes as two dlls; no matching public report; SteamVR beta 2.19.1 breaks UEVR menus (not our runtime). Bisect our own 11-05/06 changes. |
| 2026-10-26 | [UEVR runs the C++ plugins' pad callbacks BEFORE the Lua scripts'](topics/2026-10-26-uevr-runs-cpp-plugins-before-lua-on-the-pad.md) | 🆕 | One callback list, called in order; plugins register at start-up (in file-name order), Lua at the first frame and again after every "Reset scripts", so Lua always edits last. Turn the pack's pad writes off and do the whole layer in C++. Pointer in `engine-research/inbox/`. |
| 2026-10-26 | [The game's default pad layout, from public control guides](topics/2026-10-26-the-games-default-pad-layout-from-public-guides.md) | 🆕 | Inventory = View, map = D-pad Up, back = B, quick turn = RB, heal = Y (hold for the syringe); from search extracts of two guides, to confirm live. |
| 2026-10-26 | [Shadows for a hidden head and for held weapons: the standard Unreal recipe](topics/2026-10-26-shadows-for-hidden-body-parts-and-held-weapons.md) | 🆕 | `bCastHiddenShadow` and a shadow-only body copy; read the weapon mesh's and the flashlight's shadow flags live first. |
| 2026-10-26 | [Prior art, where it stands, and a command-line route for the window](topics/2026-10-26-prior-art-state-and-a-command-line-window.md) | 🆕 | Every bundled part is on its newest version; `-windowed -ResX=1280 -ResY=720` instead of trusting the ini. |

## How to add a topic

1. New file in `topics/`, named `YYYY-MM-DD-short-slug.md`.
2. One row added to the table above, newest at the top.
3. Status tags change only here, by the research session; the modding side sends its verdicts through [`inbox/`](inbox/).
