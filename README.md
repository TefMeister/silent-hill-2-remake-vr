# Silent Hill 2 (2024 remake) VR

A first-person VR mod for the Silent Hill 2 remake, in one package with its own launcher. It brings together
jbusfield's VR mod, LukasBlaster7's manual reloads, VynnGfx's performance tweaks and PureDark's AFW build of
praydog's UEVR, plus our own C++ plugin (hands, body, weapons on the body, reloads by hand) and launcher. You do not
have to download or combine anything yourself. All credit for the parts we did not make is theirs (see Shout outs
below and CREDITS.md).


Disclaimer


This is an unofficial, non-commercial fan mod. It is not made, approved or supported by Konami or Bloober Team.
You need your own legal copy of Silent Hill 2 (2024) to use it. It contains no original game files.


Caution


This mod is unfinished and still being worked on. VR in this game can cause severe motion sickness and
discomfort. Take breaks, and stop at once if you feel unwell.


Not released yet


There is no download yet. The installation steps will be added here with the first release.


About UEVR being included


This mod is built around one exact build of UEVR: PureDark's AFW build, UEVR AFW v1.0-beta.6, which is based on
praydog's UEVR. It is included in the package unchanged, exactly as downloaded from
https://github.com/PureDark/UEVR/releases/tag/UEVR_AFW_v1.0-beta.6

UEVR's own licence reads "All rights reserved". We do not own UEVR and do not claim to. We asked praydog about
including it in this mod and have had no answer, so we include it openly, named and credited, and say so here. If
praydog or PureDark asks us to stop, we will take it out straight away and point to their own downloads instead.
Please open an issue on this repo if you are one of them, or if you know of a problem with this.

Our launcher follows the steps of praydog's UEVR injector, uevr-frontend, which is under the MIT licence:
https://github.com/praydog/uevr-frontend


Main features


- Full first-person VR with hands, by jbusfield, with our own C++ plugin on top: weapons on the body, fire without
  aiming, two hands on the long guns, the map in your hand, the pistol reloaded by hand
- Manual reloads for the shotgun and rifle, by LukasBlaster7 (being reworked to work by hand)
- Engine and graphics tweaks for better performance, by VynnGfx
- One launcher that installs, updates, starts the game in VR and uninstalls


Requirements


SILENT HILL 2 (2024 remake), Steam version
A PC VR headset with an OpenXR runtime (UEVR comes with the package)


Performance


Measured 2026-10-23 on an RTX 5080 with an Intel i5-14600K, in the prison armory, one of the heaviest places in
the game: about 40 frames a second without VynnGfx's tweaks, about 41 with them. In that spot the processor,
not the graphics card, is what holds the game back. Details: modding-notes/2026-10-23-performance-baseline.md


Shout outs


jbusfield - the VR first-person mod this is built on: https://github.com/jbusfield/SH2_UEVR
Pande4360, gwizdek, markmon, Rusty Gere, lobotomy - ideas and code inside jbusfield's scripts, credited there
LukasBlaster7 - manual reloads
VynnGfx - Optimized Tweaks BASE: https://www.nexusmods.com/silenthill2/mods/42
praydog - UEVR and the Silent Hill 2 UEVR plugin: https://github.com/praydog/UEVR, and his original Silent Hill 2
  profile, whose ways of holding the map in both hands and pointing the shotgun and rifle at the other hand our
  two-handed map and long guns are built from
PureDark - the AFW build of UEVR this mod is built around and includes: https://github.com/PureDark/UEVR

If someone should be credited here and is not, please open an issue on this repo and we will fix it as soon as
possible. We honour correction and removal requests from rights holders.


What is in this repo


| Folder | What it holds |
| --- | --- |
| `mod/` | What goes into the package: the UEVR profile, the tweaks file, the licences, the player readme |
| `dev-archive/native/` | Our C++ UEVR plugin, sh2vr.dll |
| `dev-archive/launcher/` | Our launcher, SH2VR.exe, and the script that packs the zip |
| `modding-notes/` | Notes and measurements, newest first |
| `ideas/` | Mod ideas waiting to be picked |
