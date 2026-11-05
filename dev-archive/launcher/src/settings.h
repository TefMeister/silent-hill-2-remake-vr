// SH2VR launcher: every fixed name, number and place in one table (code-shape rule: numbers get names).
#pragma once
#include <cstdint>

namespace cfg {

// The game
inline constexpr wchar_t STEAM_APP_ID[]      = L"2124490";
inline constexpr wchar_t GAME_EXE[]          = L"SHProto-Win64-Shipping.exe";
inline constexpr wchar_t PROFILE_NAME[]      = L"SHProto-Win64-Shipping";       // UEVR's profile folder name
inline constexpr wchar_t PAKS_MODS_REL[]     = L"SHProto\\Content\\Paks\\~mods"; // inside the game folder
inline constexpr wchar_t SAVEGAMES_REL[]     = L"SilentHill2\\Saved\\SaveGames"; // inside %LOCALAPPDATA%
inline constexpr wchar_t INPUT_REBIND_SAV[]  = L"InputRebinding.sav";           // the player's own button changes

// Our package, beside the launcher exe
inline constexpr wchar_t PKG_PROFILE[]       = L"package\\profile";  // -> %APPDATA%\UnrealVRMod\<PROFILE_NAME>
inline constexpr wchar_t PKG_PAKS[]          = L"package\\paks";     // -> <game>\<PAKS_MODS_REL>
inline constexpr wchar_t PKG_UEVR[]          = L"uevr";              // UEVR's own files, injected from here

// Our own folder for the install record, backups and the log
inline constexpr wchar_t OUR_DIR[]           = L"SH2VR";             // inside %LOCALAPPDATA%
inline constexpr wchar_t RECORD_FILE[]       = L"install-record.tsv";
inline constexpr wchar_t BACKUPS_DIR[]       = L"backups";
inline constexpr wchar_t LOG_FILE[]          = L"launcher.log";
inline constexpr wchar_t GAME_DIR_FILE[]     = L"game-folder.txt";   // remembered --game choice

// Files the player may have tuned (UEVR's own overlay writes them): placed only when missing.
inline constexpr const wchar_t* KEEP_IF_PRESENT[] = {L"config.txt", L"cameras.txt", L"imgui.ini"};

// UEVR injection, the same order as praydog's uevr-frontend (MIT): runtime loader first, then the backend.
inline constexpr wchar_t RUNTIME_OPENXR[]    = L"openxr_loader.dll";
inline constexpr wchar_t RUNTIME_OPENVR[]    = L"openvr_api.dll";
inline constexpr wchar_t UEVR_BACKEND[]      = L"UEVRBackend.dll";
inline constexpr wchar_t UEVR_NULLIFIER[]    = L"UEVRPluginNullifier.dll";
inline constexpr char    NULLIFIER_EXPORT[]  = "nullify";
inline constexpr wchar_t RUNTIME_KEY[]       = L"Frontend_RequestedRuntime";

// The OpenXR Simulator (the pretend headset), for runs with nobody's headset on
inline constexpr wchar_t SIM_JSON_REL[]      = L"Programs\\OpenXR-Simulator\\openxr_simulator.json";  // in %LOCALAPPDATA%
inline constexpr wchar_t DIRECT_LAUNCH_ARGS[] = L"-windowed -ResX=1280 -ResY=720";

// Timings
inline constexpr uint32_t WAIT_PROCESS_MS    = 180'000;  // Steam start + the game's own start
inline constexpr uint32_t WAIT_WINDOW_MS     = 180'000;  // first-launch shader compile can take minutes
inline constexpr uint32_t INJECT_DELAY_MS    = 8'000;    // after the window shows; to tune on the first live run
inline constexpr uint32_t POLL_MS            = 500;
inline constexpr uint32_t REMOTE_THREAD_MS   = 30'000;   // LoadLibrary inside the game
inline constexpr uint32_t CLOSE_AFTER_OK_MS  = 4'000;    // a double-clicked window stays this long after success

}  // namespace cfg
