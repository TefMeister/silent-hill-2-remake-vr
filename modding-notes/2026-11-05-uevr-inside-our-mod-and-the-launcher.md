# 2026-11-05: UEVR inside our mod, and our own launcher (step 1 + a first launcher)

Home PC, Opus, no game running. Board row: "UEVR INSIDE OUR MOD + OUR OWN LAUNCHER" (Tefa 2026-11-03).

## ⚠️ UEVR's licence is NOT MIT

The board said "UEVR is MIT-licensed `[reported]`". Checked today on GitHub:

- `praydog/UEVR` `LICENSE` reads *"Copyright (c) 2022-2025 praydog. All rights reserved."* (commit "Add license
  clarification", 2025-02-19) `[verified-live 2026-11-05]`. So the UEVR program itself (UEVRBackend.dll and the rest of
  a nightly) carries no licence that lets us hand it on.
- Only `include/uevr/API.h` (the plugin headers our `sh2vr.dll` builds against) says it is MIT and separate from the
  rest `[verified-live 2026-11-05]`. Our plugin is fine.
- `praydog/uevr-frontend` (the injector program, UEVRInjector.exe) is MIT `[verified-live 2026-11-05]`. Our launcher
  follows its steps (written fresh, not copied).
- PureDark's AFW build: no licence checked; it stays optional, as decided 2026-09-19.

Tefa asked praydog on Discord to reply only if they object, and took the silence as a yes. That is Tefa's call. The
safer shape that still gives players one package: **the launcher downloads the UEVR nightly from praydog's own
GitHub release on first start** (pinned, checked by its sha256), so we never hand UEVR on ourselves. The launcher
already takes UEVR from a folder (`uevr/` beside it, or `--uevr <folder>`), so either way works without changes
elsewhere. Waiting on Tefa's choice.

## What the launcher must place (from the home PC's install today)

| What | Where it goes | Source / owner | Ship? |
| --- | --- | --- | --- |
| UEVR: UEVRBackend.dll, openxr_loader.dll, openvr_api.dll, LuaVR.dll, UEVRPluginNullifier.dll | stays in the launcher's own `uevr\` folder, loaded into the game from there | praydog (all rights reserved) | ⚠️ Tefa decides: bundle or download |
| The profile: config.txt, cameras.txt, user_script.txt, data\*.json, scripts\ (+ libs) | %APPDATA%\UnrealVRMod\SHProto-Win64-Shipping | jbusfield (MIT) + LukasBlaster7 (permission) + our edits | yes (repo `mod/`) |
| plugins\sh2r.dll | profile\plugins | praydog's SH2R-UEVR (no licence file; Tefa: use it) | yes, as now |
| plugins\uevr_utils.dll | profile\plugins | jbusfield's pack | yes |
| plugins\sh2vr.dll | profile\plugins | ours | yes (not yet in `mod/`: build.sh deploys it straight to the profile) |
| ~SH2optimizedTweaksBASE_P.pak | game\SHProto\Content\Paks\~mods | VynnGfx (permission) | yes |
| plugins\uevr_mcp.dll | profile\plugins | dev tool | NO |
| openxr_loader.dll beside the game's exe | game\SHProto\Binaries\Win64 | dev-only fix for injecting through the UEVR MCP | NO (the launcher loads the runtime first, as the injector does) |
| 21 Sacraments patch (version.dll, version.ini, scripts\silenttraverse.asi) | beside the game's exe | its author, not asked yet | NO |
| cache\, imgui.ini, log.txt, uobjecthook\, breadcrumb json | profile | made by UEVR at run time | NO |
| Music at 0 (PlayerProfile.sav) | saves | our test rule only | NO |

## What must be vanilla, and how

| Setting | Where | What the launcher does |
| --- | --- | --- |
| The player's button changes | %LOCALAPPDATA%\SilentHill2\Saved\SaveGames\InputRebinding.sav | backs it up and removes it before every launch (our button layer is built on the vanilla pad). `[hypothesis]` the game then starts on its defaults: prove on the first flat run |
| Screen effects for VR (motion blur, Panini; then film grain, lens flares, colour fringing, depth of field) | GameUserSettings.ini GFXSettings blob (`Adv_MotionBlur`, `Panini`, `Adv_LensFlares`) and engine cvars | NOT in the launcher: better done by sh2vr.dll each launch through cvars, so nothing has to be put back on uninstall. New board row. |
| Window mode / resolution | GameUserSettings.ini | left to the player (only our test runs need 1280×720) |

## The launcher, first version (`dev-archive/launcher/`)

`SH2VR.exe`, plain C++ (CMake, MSVC, static runtime), 6 small files. `[compile-verified 2026-11-05]`, no warnings at /W4.

- **play** (the default): find the game through Steam (registry → libraryfolders.vdf → appmanifest_2124490.acf), install
  or update the package, reset the button file, start the game with `steam://rungameid/2124490`, wait for its window,
  wait 8 s (`INJECT_DELAY_MS`, to tune), then load UEVR the way praydog's injector does: (optional nullifier) → the
  runtime loader (openxr_loader.dll; `--openvr` for SteamVR) → write `Frontend_RequestedRuntime` into config.txt →
  UEVRBackend.dll. Then it exits, so nothing keeps Discord showing "playing" after the game closes.
- **attach**: the same into a game already running. **install**, **uninstall**, **status**.
- **Install record** (%LOCALAPPDATA%\SH2VR\install-record.tsv): every file added, replaced (backed up) or moved away
  (backed up), and every folder made. Uninstall walks it backwards. config.txt / cameras.txt / imgui.ini are placed only
  when missing, so a player's own UEVR settings survive updates.
- **Tested in a throw-away folder** (`--test-root`, launches nothing) `[measured 2026-11-05, n=1]`: fresh install
  (100 files added, 1 replaced and backed up, the player's config kept, the button file moved away), a second install
  changed nothing, uninstall left every file byte-for-byte as before (md5 of the whole tree). Fixed on the way: backup
  names mirrored the whole path and ran past Windows' 260-character limit.
- `status` on the real PC found the game through Steam `[measured 2026-11-05]`.
- **Not run yet:** play / attach against the real game (needs a flat run: `[FLAT]` row).

## Next

1. Tefa: bundle UEVR, or let the launcher download it from praydog's release?
2. [FLAT] first real run: `SH2VR.exe play --uevr "<UEVR AFW b6 folder>"` on the home PC with the simulator or the
   headset; check UEVR comes up, the 8 s wait is enough, and the game starts on default controls.
3. [PD] build the package: `build.sh` puts sh2vr.dll into `mod/` too; a pack script lays out SH2VR.exe + package\ +
   uevr\ (or the download step) + licences + credits + the readme in Tefa's layout.
4. [PD] sh2vr.dll sets the VR screen-effect cvars every launch.
