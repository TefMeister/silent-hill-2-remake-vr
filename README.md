# Silent Hill 2 (2024 remake) — working toward VR

Notes and tooling for playing Bloober Team's **Silent Hill 2** remake in a VR headset, as a
self-contained UEVR profile that depends on nobody else's mod being installed.

## What is here

| Folder | What it holds |
| --- | --- |
| [`engine-research/`](engine-research/) | [`ENGINE-DOSSIER.md`](engine-research/ENGINE-DOSSIER.md) — what is actually known about the game and its engine, with confidence tags |
| [`dev-archive/`](dev-archive/) | Working source, profile files, scripts and reverse-engineering evidence |

Other folders (`modding-notes/`, `external-research/`, `mod/`) appear as they are earned.

## Status

**Day one. Nothing built, nothing run, the game has never been launched for this project.**

The plan exists and the route is chosen; that is all. See
[`engine-research/ENGINE-DOSSIER.md`](engine-research/ENGINE-DOSSIER.md) for what is known versus
what is merely assumed — and note how much of it is currently assumed.

## Why this game, and why it is different from everything else here

**It is Unreal Engine 5**, and on this account that matters more than anything else about a game.

Every other project here is a bespoke or dead engine that has to be reverse engineered from nothing —
proxy DLLs, debuggers, hunting for camera matrices in memory, a great deal of work before a single
pixel moves. Unreal 4 and 5 have **[UEVR](https://github.com/praydog/UEVR)** (praydog), a
general-purpose VR injector that already understands how Unreal delivers cameras, projections and
stereo, with documentation and per-game settings.

**So the things that take months elsewhere are plausibly a starting point here rather than a
destination.**

⚠️ **But the order of the work inverts, and that is worth expecting.** Elsewhere the fight is to get
*any* stereo image and everything after is polish. Here the image may come early — and then all of
the interesting work is still ahead: hands, comfort, scale, the UI, and in this game specifically the
fog. Easy start, not an easy project.

## The goal, in one paragraph

A harder, quieter *Silent Hill 2* played in a headset. Fewer enemies that can genuinely kill you, no
radio warning you they are coming, and everything you carry hanging off your body instead of sitting
in a menu — pistol and shotgun and rifle on hip and shoulders, the map on the other hip, ammo reached
with the other hand, and the torch stowed **above your head**, so putting it away still lights the
fog in front of you.

Ideas for this game are collected and judged before they are built. That happens in a private idea
repository, not here; only settled work arrives in this repo.

## Approach

1. **Study what already exists.** There are published UEVR profiles for this game. Reading other
   people's public work is how you avoid rediscovering what the game does — and it is free, needing
   no game running at all.
2. **Write our own, depending on none of it.** A profile that quietly requires somebody else's mod to
   be installed is not something we can release. Whatever is learned gets reimplemented.
3. **Then the hard half** — body-relative holsters, comfort, scale, and the fog.

⚠️ **One decision is explicitly not made yet:** which UEVR build to stand on. A modified fork has
been suggested; nothing about it has been examined here — what it is, whether it can be built on, how
it is distributed, or what depending on it would mean for releasing a mod that redistributes nothing.
**Until that is checked, the plan assumes plain upstream UEVR.**

⚠️ **Also unchecked:** whether this release carries anti-tamper that interferes with injection. A
recent commercial title usually does something.

## What this is not

- **Not a VR port** of the game, and not affiliated with Bloober Team, Konami, or the authors of any
  existing VR profile.
- **Not a repackage** of anybody else's mod, profile or fork.
- **No game content.** No assets, executables or archives — nothing from the game. You need your own
  legitimate copy.
- **Unfinished**, and at the time of writing not even started. Nothing here is a supported product.

## Caution

Experimental and unfinished. Any VR build that comes out of this may cause **severe motion sickness
and discomfort**, and *Silent Hill 2* in a headset is not a gentle experience to begin with. That
warning will only be removed for a build once it has been played through and confirmed comfortable.

## Credits

- **Bloober Team**, who made the remake, **Konami**, who published it, and **Team Silent**, whose
  original this is.
- **[praydog](https://github.com/praydog)** for **UEVR**, without which this project would be a
  multi-month reverse-engineering effort instead of a profile.
- **The authors of the existing Silent Hill 2 UEVR profiles and mods**, whose public work is the
  starting point for understanding this game. They will be named individually here once their work
  has actually been studied rather than merely cited — crediting people accurately matters more than
  crediting them quickly.

If you should be credited here and are not, or want something changed or removed, please get in touch
and it will be put right as fast as we can.

## Legal

A non-commercial fan project. It requires you to own a legitimate copy of the game and redistributes
no original assets.
