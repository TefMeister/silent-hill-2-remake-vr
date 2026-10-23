# Silent Hill 2 (2024 remake) VR

A first-person VR mod for the Silent Hill 2 remake, in one package. It brings together jbusfield's VR mod,
LukasBlaster7's manual reloads and VynnGfx's performance tweaks, so you do not have to download and combine
them yourself. Each of them gave permission for their work to be included. All credit for those parts is
theirs (see Shout outs below and CREDITS.md).

Next, the plan is to rewrite the mod in C++ so it runs faster.


Disclaimer


This is an unofficial, non-commercial fan mod. It is not made, approved or supported by Konami or Bloober Team.
You need your own legal copy of Silent Hill 2 (2024) to use it. It contains no original game files.


Caution


This mod is unfinished and still being worked on. VR in this game can cause severe motion sickness and
discomfort. Take breaks, and stop at once if you feel unwell.


Installation instructions


STEP 1 - The game

1.1  Install SILENT HILL 2 (2024) from Steam (only tested with the Steam version).

1.2  Find the game folder: in Steam, right-click the game > Manage > Browse local files. This is the folder
that contains SHProto.exe. Every "game folder" below means this folder.

1.3  Start the game once, reach the main menu, and quit.


STEP 2 - UEVR

2.1  Get PureDark's AFW build of UEVR, version UEVR_AFW_v1.0-beta.6 (the one this package is tested with). It is
based on praydog's UEVR:
https://github.com/PureDark/UEVR/releases/tag/UEVR_AFW_v1.0-beta.6

2.2  Unpack it into its own folder anywhere you like.


STEP 3 - This mod

3.1  Open the "mod" folder of this package.

3.2  Copy the folder "UnrealVRMod\SHProto-Win64-Shipping" into this folder (paste the line into the Windows
Explorer address bar):
          %APPDATA%\UnrealVRMod
If Windows asks to merge folders, say yes.

3.3  Copy the folder "Paks\~mods" into this folder inside the game folder:
          SHProto\Content\Paks
(This is VynnGfx's Optimized Tweaks. It is optional.)


STEP 4 - Play

4.1  Start your VR runtime (for example Virtual Desktop) with OpenXR.

4.2  Start the game.

4.3  Start UEVRInjector.exe, choose SHProto-Win64-Shipping, choose OpenXR, and press Inject.

4.4  Tested with these game settings: graphics quality Epic, DLSS on Quality, motion blur off.


UNINSTALL

Delete the folder %APPDATA%\UnrealVRMod\SHProto-Win64-Shipping and the file
SHProto\Content\Paks\~mods\~SH2optimizedTweaksBASE_P.pak in the game folder.


Main features


- Full first-person VR with hands, by jbusfield
- Manual reloads, by LukasBlaster7: pistol magazine, shotgun shells and pump, rifle bolt and shells
- Engine and graphics tweaks for better performance, by VynnGfx


Requirements


UEVR (tested with PureDark's AFW build)


Performance


Measured 2026-10-23 on an RTX 5080 with an Intel i5-14600K, in the prison armory, one of the heaviest places in
the game: about 40 frames a second without VynnGfx's tweaks, about 41 with them. In that spot the processor,
not the graphics card, is what holds the game back. Details: modding-notes/2026-10-23-performance-baseline.md


Shout outs


jbusfield - the VR first-person mod this is built on: https://github.com/jbusfield/SH2_UEVR
LukasBlaster7 - manual reloads
VynnGfx - Optimized Tweaks BASE: https://www.nexusmods.com/silenthill2/mods/42
praydog - UEVR and the Silent Hill 2 UEVR plugin: https://github.com/praydog/UEVR
PureDark - the AFW build of UEVR: https://github.com/PureDark/UEVR

If someone should be credited here and is not, please open an issue on this repo and we will fix it as soon as
possible. We honour correction and removal requests from rights holders.


What is in this repo


| Folder | What it holds |
| --- | --- |
| `mod/` | The package: the UEVR profile and the tweaks file |
| `modding-notes/` | Notes and measurements, newest first |
| `ideas/` | Mod ideas waiting to be picked |
