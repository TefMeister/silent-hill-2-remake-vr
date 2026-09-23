# Build our own UEVR profile — studying two existing ones, standing on an AFW fork, depending on neither

Order: 6001
From: mod-ideas `games/silent-hill-2-remake.md` (<https://github.com/TefMeister/mod-ideas/blob/main/games/silent-hill-2-remake.md>), copied 2026-09-23

`[raw]` · `[looks doable]` for the profile; `[not judged]` for the AFW foundation — *none of the three named things has been looked at from here*

> "add a repo for Silent Hill 2 Remake, we are gonna study jbusfield uevr profile, CharlotteLiu mod
> that builds on top of it and build our profile without their dependancies, using PureDark's AFW
> version of UEVR as a foundation to build on"

Verbatim record: [`inbox/2026-09-18-sh2-repo-and-uevr-profile-foundation.md`](../inbox/2026-09-18-sh2-repo-and-uevr-profile-foundation.md)

**This is the first thing filed for this game that is a plan rather than a wish**, and it answers the
open question at the top of this page — *do you actually care about the VR side, or about difficulty
and dread?* The answer is VR, and it arrived with a route already chosen.

**The shape of it, in plain terms:** read two people's existing work to learn what they discovered
about this game, then write our own from scratch so nothing of ours needs their files installed to
run. That is the same pattern used everywhere else on this account — study, then reimplement — and it
is what keeps a mod ours to release.

**What it'd take:**

- **The repo** — trivial, and the standing bootstrap already says exactly what a new game repo gets.
  ⚠️ **Not created yet: this reads like a go-ahead, but creating a project is a graduation, and only
  you settle those.** One word and it exists.
- **Studying the two profiles** — this is `/gr` work and needs no game running at all. Reading a
  published UEVR profile is reading config and Lua, not reverse engineering.
- **"Without their dependencies"** — the genuinely valuable constraint, and the reason to write it
  down now rather than later. A profile that quietly needs somebody else's mod present is not
  releasable. Worth checking early *which* parts of the existing work are settings (free to learn
  from) and which are their own code (must be replaced).
- **⚠️ PureDark's AFW build as the foundation — the one part with a real question in it.** Nothing
  about it has been checked from here: what it is, whether it is a fork we can build on, how it is
  distributed, and what depending on it would mean for releasing a mod under the usual
  redistribute-nothing rules. **`[not judged]` — do not plan around this until a `/gr` pass has
  actually looked.** The rest of the idea stands on plain UEVR regardless, which is the safe reading.
