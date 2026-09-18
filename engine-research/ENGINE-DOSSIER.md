# Engine dossier — Silent Hill 2 (2024 remake)

What is actually known, and how well. Confidence tags follow the house vocabulary:
`[verified-live]`, `[measured]`, `[verified-numerically]`, `[compile-verified]`,
`[inferred-static]`, `[reported]`, `[hypothesis]`, `[disproved]`.

⚠️ **Read the tags on this page before planning anything.** This dossier was opened on 2026-09-18
with the game never launched, never owned-and-checked from here, and no file of it ever opened. Almost
everything below is `[reported]` or `[hypothesis]`. **There is currently not one `[verified-live]`
line on this page**, and that is the single most important fact about it.

## 1. The game

*Silent Hill 2*, Bloober Team, published by Konami, released October 2024. A remake of Team Silent's
2001 original.

- **Engine: Unreal Engine 5** `[reported]` — widely stated publicly; not confirmed from the shipped
  files here.
- **Platforms:** PC (Steam, Epic) and PlayStation 5 `[reported]`.
- **Owned / installed on either machine?** Not checked. Nothing in this project has looked at a real
  install.
- **Which UE5 minor version** the shipping build uses is unknown, and it matters: UEVR's per-version
  behaviour differs. `[hypothesis]` that it is within UEVR's supported range — untested.

## 2. Why this project exists at all

UE4 and UE5 have **UEVR** (praydog), a general-purpose injector that already knows how Unreal
delivers cameras, projections and stereo. The standing rule on this account about picking new games
is to **name the engine first**, because a UE4/UE5 pick is far cheaper than a bespoke engine.

This is the only Unreal project on the account, and therefore the cheapest VR target on it
`[inferred-static]` — inferred from how much cheaper UEVR makes the first stereo image, not from any
measurement on this game.

## 3. The route in — what has been named, and what has been checked

Three specific things were named by Tefa on 2026-09-18 as the starting point. **None of the three has
been looked at from here.** Listing them is not endorsing them.

| Named | What it is claimed to be | Status here |
| --- | --- | --- |
| **jbusfield's UEVR profile** for this game | an existing published profile | `[reported]` — not located, read, or tested |
| **CharlotteLiu's mod** built on top of it | a mod layered on that profile | `[reported]` — not located or read |
| **PureDark's "AFW" build of UEVR** | a modified UEVR to build on | `[reported]` — **`[not judged]` for buildability.** Nothing known here about what it is, whether it is a fork we can build on, how it is distributed, or what depending on it would mean under the redistribute-nothing rule |

**The stated intention:** study the first two, depend on neither, and reimplement. That is the same
study-then-reimplement pattern used on every other project here, and it is what keeps a mod ours to
release.

⚠️ **Do not plan around the AFW build until a `/gr` pass has actually examined it.** The rest of the
plan works on plain upstream UEVR, which is the safe default and what everything below assumes.

## 4. Anti-tamper — unchecked, and a real risk

Whether the shipping build carries anti-tamper that interferes with injection has **not been
checked** `[hypothesis]`. A recent commercial title usually does something. This is worth resolving
early because it is the one finding that could invalidate the whole approach rather than merely cost
time.

## 5. Windowed mode — owed on first launch

Standing rule: on a game's first live session, before other work, prove it runs as shipped, then runs
with our smallest file present, then **make it playable in a 1280×720 window** — found through the
game's own setting (config file, registry, command line or menu), measured, backed up, and recorded
here.

**Not done. The game has never been launched for this project.** Prefer a config or command-line
route: in-game "keep these settings?" countdowns revert when nobody is there to click.

## 6. What the mod is meant to become

Design detail lives in the idea repository, not here. In summary, two threads are settled enough to
name:

- **Difficulty and dread** — fewer enemies, stronger and faster, placed differently; the radio silent
  until you are already being hit; combat music only once it has started; Pyramid Head driven off by
  damage rather than by a scripted beat (he does not die — he walks out of the door).
- **Everything on your body, nothing in a menu** — pistol right hip, map left hip, shotgun right
  shoulder, rifle left shoulder, all taken with the right hand; ammo at the left hip taken with the
  left hand; the torch stowed centred **above the head**, taken and held with the left hand.

⚠️ **Whether a UEVR profile can reach the actions these need** — swap weapon, use item — is the
central unknown for the second thread `[hypothesis]`. Knowing where head and hands are is what UEVR
is for; making the game act on a gesture is the part that has to be proven.

## 7. Dead ends

None yet. **This section earns its keep only when something has actually been tried**, and nothing
has. Research sessions read this section first, so leaving it honestly empty is better than filling it
with guesses.
