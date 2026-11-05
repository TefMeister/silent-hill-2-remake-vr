// Shared helpers: logging, wide/narrow text, the folders everything goes to.
#pragma once
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

// Where every destination lives. A test root redirects ALL of them into one throw-away folder,
// so the launcher can be tested without touching the real game, profile or saves.
struct Places {
    fs::path launcher_dir;   // the folder holding SH2VR.exe and package/, uevr/
    fs::path game_dir;       // the Steam install of Silent Hill 2 (may be empty if not found)
    fs::path profile_dir;    // %APPDATA%\UnrealVRMod\SHProto-Win64-Shipping
    fs::path savegames_dir;  // %LOCALAPPDATA%\SilentHill2\Saved\SaveGames
    fs::path our_dir;        // %LOCALAPPDATA%\SH2VR
    fs::path uevr_dir;       // the UEVR build to inject (default launcher_dir\uevr)
    bool test = false;
};

void log_open(const fs::path& file);
void logf(const wchar_t* fmt, ...);

std::wstring widen(const std::string& s);
std::string narrow(const std::wstring& s);
std::wstring read_text_file(const fs::path& p);       // UTF-8 (BOM tolerated) -> wide
bool write_text_file(const fs::path& p, const std::wstring& text);  // wide -> UTF-8, no BOM
std::wstring now_stamp();                              // 2026-11-05_1430
fs::path known_folder_roaming();
fs::path known_folder_local();
fs::path exe_dir();
