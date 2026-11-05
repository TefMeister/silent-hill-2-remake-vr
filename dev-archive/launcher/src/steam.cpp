// Finds the Silent Hill 2 folder through Steam: the Steam folder from the registry, every library listed in
// libraryfolders.vdf, the one holding appmanifest_<id>.acf, and that manifest's "installdir".
#include "steam.h"

#include "settings.h"

#include <windows.h>

#include <regex>

static fs::path steam_root() {
    wchar_t buf[MAX_PATH * 2];
    DWORD size = sizeof(buf);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath", RRF_RT_REG_SZ, nullptr, buf, &size) ==
        ERROR_SUCCESS)
        return fs::path(buf).make_preferred();
    return {};
}

// Every "key" "value" pair whose key matches, with the vdf's doubled backslashes undone.
static std::vector<std::wstring> vdf_values(const std::wstring& text, const std::wstring& key) {
    std::vector<std::wstring> out;
    std::wregex re(L"\"" + key + L"\"\\s+\"([^\"]*)\"", std::regex::icase);
    for (std::wsregex_iterator it(text.begin(), text.end(), re), end; it != end; ++it) {
        std::wstring v = (*it)[1];
        std::wstring clean;
        for (size_t i = 0; i < v.size(); ++i) {
            if (v[i] == L'\\' && i + 1 < v.size() && v[i + 1] == L'\\') ++i;
            clean += v[i];
        }
        out.push_back(clean);
    }
    return out;
}

fs::path find_game_dir() {
    fs::path root = steam_root();
    if (root.empty()) return {};
    std::vector<fs::path> libraries{root};
    for (auto& p : vdf_values(read_text_file(root / L"steamapps" / L"libraryfolders.vdf"), L"path"))
        libraries.emplace_back(fs::path(p).make_preferred());

    const std::wstring manifest = std::wstring(L"appmanifest_") + cfg::STEAM_APP_ID + L".acf";
    for (auto& lib : libraries) {
        fs::path acf = lib / L"steamapps" / manifest;
        std::error_code ec;
        if (!fs::exists(acf, ec)) continue;
        auto dirs = vdf_values(read_text_file(acf), L"installdir");
        if (dirs.empty()) continue;
        fs::path game = lib / L"steamapps" / L"common" / dirs.front();
        if (fs::exists(game, ec)) return game;
    }
    return {};
}

bool looks_like_game_dir(const fs::path& dir) {
    std::error_code ec;
    return fs::exists(dir / L"SHProto" / L"Binaries" / L"Win64" / cfg::GAME_EXE, ec);
}
