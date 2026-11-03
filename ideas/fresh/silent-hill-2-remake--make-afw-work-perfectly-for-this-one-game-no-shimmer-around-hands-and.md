# Make AFW work perfectly for this one game: no shimmer around hands and objects

Order: 6011
From: the ideas repo, `games/silent-hill-2-remake.md` (<https://github.com/TefMeister/mod-ideas/blob/main/games/silent-hill-2-remake.md>), copied 2026-11-03

`[raw]` · `[looks doable]` — ⚠️ *partly checked 2026-11-03 (the files and settings, not the game): the cause below is a
hypothesis until a headset test confirms it*

> "do you think it's possible to perfect AFW for just this game, it is a universal tool but in our mod, can you make
> sense of what makes AFW work and get it to work better than it is right now, there is shimmering around hands and
> objects" — 2026-11-03

AFW is PureDark's frame-warp add-on for UEVR: it draws fewer real frames and builds the missing ones by sliding the
last picture along, using each pixel's movement and distance. Shimmer appears where it guesses wrong: at the edges of
things that move on their own (the hands, the gun) and in the gaps they uncover.

**What it'd take:** the add-on itself is closed (a ready-made file, no source), so it cannot be rewritten. What we can
fix is what the game hands it. Three levers, cheapest first:
- its own settings in our profile (the "fix object motion" options are already on; the strength and range can be tuned
  per game and measured);
- the engine's own switches that make every moving thing, the hands included, report its movement properly;
- in our C++ plugin, moving the hands at the moment the engine expects, so their movement is recorded instead of looking
  like a jump. The likely culprit: UEVR moves the hands late, after the engine has noted where they were, so AFW sees
  them as not moving and smears their edges `[hypothesis]`.
The gaps uncovered behind a moving hand can be made smaller, never zero: frame warping always has to invent them.

Verbatim record: [`inbox/2026-11-03-sh2-perfect-afw-no-shimmer.md`](../inbox/2026-11-03-sh2-perfect-afw-no-shimmer.md)
