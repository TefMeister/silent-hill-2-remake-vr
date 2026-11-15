// upscaler.cpp -- see upscaler.h
#include "upscaler.h"

#include <fstream>
#include <sstream>
#include <system_error>

#include "settings.h"
#include "upscaler_text.h"

namespace {
fs::path settings_file(const Places& pl) { return pl.savegames_dir.parent_path() / cfg::GAME_SETTINGS_REL; }

bool read_bytes(const fs::path& p, std::string& out) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    std::ostringstream s;
    s << f.rdbuf();
    out = s.str();
    return true;
}
}  // namespace

bool check_upscaler(const Places& pl) {
    const fs::path ini = settings_file(pl);
    std::string text;
    if (!read_bytes(ini, text)) { logf(L"(the game's graphics settings are not there yet: the game writes them at its first start)"); return true; }
    std::vector<std::string> said;
    const std::string fixed = upscaler::fix(text, said);
    if (fixed == text) { logf(L"Graphics settings: DLSS on, motion blur off"); return true; }
    std::error_code ec;
    const fs::path bk_dir = pl.our_dir / cfg::BACKUPS_DIR / now_stamp();
    fs::create_directories(bk_dir, ec);
    if (!fs::copy_file(ini, bk_dir / ini.filename(), fs::copy_options::overwrite_existing, ec)) {
        logf(L"! could not back up the game's graphics settings, so they were left as they are");
        return false;
    }
    std::ofstream f(ini, std::ios::binary | std::ios::trunc);
    f.write(fixed.data(), (std::streamsize)fixed.size());
    if (!f) { logf(L"! could not write the game's graphics settings back"); return false; }
    for (const auto& s : said) logf(L"Graphics settings: %s", widen(s).c_str());
    logf(L"(the old settings file is kept in the backups)");
    return true;
}
