# Switch off what a headset does not need, to win back frame rate

Order: 6003
From: the ideas repo, `games/silent-hill-2-remake.md` (<https://github.com/TefMeister/mod-ideas/blob/main/games/silent-hill-2-remake.md>), copied 2026-09-28

`[raw]` · `[looks doable]` — ⚠️ *not checked against this game: the settings below are standard Unreal Engine ones, and
whether SH2 honours each of them is untested*

> "while modding the game, make it run better, by removing something that does not matter in VR and saves
> performance, if this is even possible" — 2026-09-28

In VR the game draws every frame twice, once per eye, so anything expensive that adds nothing in a headset costs
double. Several of the remake's camera effects are made for a flat screen and are either invisible, wrong or
uncomfortable in VR:

- **Motion blur** and **depth of field**: your eyes do this themselves in a headset; the fake version blurs things
  you are looking straight at, and is a known cause of motion sickness.
- **Film grain, chromatic aberration, lens flares, vignette**: "camera lens" effects; there is no camera in VR.
- **Screen-space reflections**: calculated from the flat picture, so each eye gets a slightly different, wrong
  reflection, and they cost a lot.
- Possibly **lower quality for effects far away** (volumetric fog detail, shadow distance), since the headset's
  resolution hides them. That one needs care, because the fog is Silent Hill's identity.

**What it'd take:** SH2 is Unreal Engine 5.1, and UEVR (which already runs it here in stereo) has a console for the
engine's own settings, the `r.` switches (e.g. motion blur, depth of field, grain, lens effects). So it is a list
of switches in our profile, tried one at a time with the frame rate measured before and after each, and a
headset look to confirm nothing important went missing. No reverse engineering needed for the first round.
⚠️ Not checked: whether each switch works in this game, and how much each one saves.

**Tefa's direction, the same day:** *"there are many tools and options out there, we just need to study them and
actually work some of these fixes into our mod on a native level, rather than putting a plaster on it."* So the console
switches are only the way to MEASURE what each effect costs; the fixes themselves go into our native C++ plugin, set
by the mod where the engine sets them, and the existing tools that do similar things get studied first (studied, not
copied). Verbatim: [`inbox/2026-09-28f-sh2-native-fixes-not-plasters.md`](../inbox/2026-09-28f-sh2-native-fixes-not-plasters.md)

Verbatim record: [`inbox/2026-09-28e-sh2-drop-what-vr-does-not-need.md`](../inbox/2026-09-28e-sh2-drop-what-vr-does-not-need.md)

---

_Other categories appear as they arrive — gameplay, weapons, visuals, audio, UI, level design. Nothing is missing; none of them have been needed yet._

**To add one:** `[sh2] your idea` — anywhere, any time.

Chosen by Tefa: 2026-09-28
