# Cutscenes fade in and out, instead of cutting straight to the screen

Order: 6003
From: the ideas repo, `games/silent-hill-2-remake.md` (<https://github.com/TefMeister/mod-ideas/blob/main/games/silent-hill-2-remake.md>), copied 2026-11-13

`[raw]` · `[looks doable]` — ⚠️ *not checked against the game: judged from how build 13 switches today*

> "fade in and out cutscenes" — 2026-11-13 (idea 1)

Since build 8 a cutscene appears on a big flat screen the moment it starts, and play snaps back the moment it ends.
This asks for a short fade at both ends: the world dims out, the screen comes up, and the reverse on the way back.

**What it'd take (unchecked):** our plugin already knows the moment a cutscene starts and ends (it is what switches the
screen on); a fade to black and back over a fraction of a second at each switch, drawn by the VR layer or the game's
own camera fade. The one thing to watch: the fade must not delay the switch itself, which runs off the game thread on
purpose (doing it in the tick crashed the game).

Verbatim record: [`inbox/2026-11-13-sh2-re2-re3-cutscene-fades-fog-flat-screen.md`](../inbox/2026-11-13-sh2-re2-re3-cutscene-fades-fog-flat-screen.md)
