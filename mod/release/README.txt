Silent Hill 2 VR {VERSION}


Description


A full VR mod for SILENT HILL 2 (2024 remake) on PC. Everything is in this one package: UEVR (PureDark's AFW build),
the VR profile, the hand and body mod, the reloads by hand and the performance tweaks. Our own launcher installs it,
starts the game and brings up VR. Nothing else to download.

CAUTION: this mod is still being worked on. It is unfinished, things may break, and VR like this can cause severe
motion sickness and discomfort. Take breaks, and stop at once if you feel unwell.

This is a free fan mod. It contains no game files: you need your own copy of SILENT HILL 2 from Steam.


Installation instructions


STEP 1 - The game


1.1  Install SILENT HILL 2 (2024) from Steam (only tested with the Steam version).

1.2  Start the game once, reach the main menu, and quit. (This lets Steam and the game finish their setup.)


STEP 2 - VR


2.1  Start your VR runtime with OpenXR (tested with Virtual Desktop on a Quest 3).
- SteamVR also works through OpenXR. To use OpenVR instead, start the mod from the Windows command line with: SH2VR.exe --openvr


STEP 3 - This mod


3.1  Unzip this package into any folder of your own (not into the game folder), for example Documents\SH2VR.

3.2  Double-click SH2VR.exe.
- The first time, it finds the game through Steam, installs the VR profile and the tweaks file, and sets the game's
  controls back to their defaults (the mod's button layout is built on the default controls). Your old button
  settings are kept in a backup.
- Then it starts the game through Steam and brings up VR about 8 seconds after the game's window shows.
- If it cannot find the game, it says so. Start it once from the command line with the game's folder:
  SH2VR.exe --game "C:\Program Files (x86)\Steam\steamapps\common\SILENT HILL 2"

3.3  The first start after installing compiles shaders (about a minute, a "Compiling shaders" screen). Wait for it.


STEP 4 - Play


4.1  Every time: put the headset on, start your VR runtime, double-click SH2VR.exe. Do not start the game from
Steam yourself.

4.2  Each start checks the mod's files and puts back anything that changed, so updating the mod is: unzip the new
package over the old one and double-click SH2VR.exe.


UNINSTALL


Close the game, then double-click Uninstall.bat in this package. It takes out every file the mod added, puts back
every file it replaced (your button settings too), and leaves the game as it was. Then delete this folder.


Main features


- Real hands with fingers, the gun in your hand, the arms following the controllers.
- Weapons carried on the body: grab them from the hips and shoulders with the grip button.
- Fire without aiming: the trigger shoots wherever the gun points.
- Reload the pistol by hand: a new magazine from the left hip, the slide racked by hand.
- Two hands on the long guns, and the map held in your hand.
- Still being worked on: the shotgun pump, the rifle bolt, the left hand's finger shape on the long guns, and the healing items shown in the hand.
- VR button layout built on the game's default controls.
- Performance tweaks included.
- Motion blur, depth of field, film grain, colour fringing, lens flares and lens distortion switched off in VR.
- The left hand holds the shotgun and rifle front where the game itself holds them.


Requirements


SILENT HILL 2 (2024 remake), Steam version
A PC VR headset with an OpenXR runtime
Windows 10 or 11, 64-bit


Shout outs


jbusfield - the VR first-person mod for Silent Hill 2 this whole mod is built on (MIT licence): https://github.com/jbusfield/SH2_UEVR
Pande4360, gwizdek, markmon, Rusty Gere, lobotomy - ideas and code inside jbusfield's scripts
LukasBlaster7 - the manual reloads on top of jbusfield's mod: https://buymeacoffee.com/lukasblaster
VynnGfx - Optimized Tweaks BASE: https://www.nexusmods.com/silenthill2/mods/42
praydog - UEVR and the Silent Hill 2 UEVR plugin: https://github.com/praydog/UEVR
PureDark - the AFW build of UEVR this mod is built around: https://github.com/PureDark/UEVR

If someone should be credited here and is not, please open an issue on https://github.com/TefMeister/silent-hill-2-remake-vr
and we will fix it as soon as possible. About UEVR being included: see licences\NOTICE-UEVR.txt.
