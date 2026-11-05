#include "install.h"

#include "settings.h"
#include "steam.h"

#include <windows.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

namespace {

// One line of the install record. A = we added the file, R = we replaced it (backup holds the old one),
// M = we moved it away (backup holds it), D = we created the folder.
struct Entry {
    wchar_t kind;
    fs::path dest;
    fs::path backup;
};

fs::path record_path(const Places& pl) { return pl.our_dir / cfg::RECORD_FILE; }

std::vector<Entry> load_record(const Places& pl) {
    std::vector<Entry> out;
    std::wistringstream in(read_text_file(record_path(pl)));
    std::wstring line;
    while (std::getline(in, line)) {
        if (line.size() < 3 || line[1] != L'\t') continue;
        Entry e{line[0], {}, {}};
        std::wstring rest = line.substr(2);
        size_t tab = rest.find(L'\t');
        e.dest = rest.substr(0, tab);
        if (tab != std::wstring::npos) e.backup = rest.substr(tab + 1);
        out.push_back(e);
    }
    return out;
}

bool save_record(const Places& pl, const std::vector<Entry>& rec) {
    std::wstring text;
    for (auto& e : rec) {
        text += e.kind;
        text += L'\t';
        text += e.dest.wstring();
        if (!e.backup.empty()) text += L'\t' + e.backup.wstring();
        text += L'\n';
    }
    std::error_code ec;
    fs::create_directories(pl.our_dir, ec);
    return write_text_file(record_path(pl), text);
}

bool same_bytes(const fs::path& a, const fs::path& b) {
    std::error_code ec;
    if (fs::file_size(a, ec) != fs::file_size(b, ec) || ec) return false;
    std::ifstream fa(a, std::ios::binary), fb(b, std::ios::binary);
    std::vector<char> ba(1 << 16), bb(1 << 16);
    while (fa && fb) {
        fa.read(ba.data(), (std::streamsize)ba.size());
        fb.read(bb.data(), (std::streamsize)bb.size());
        if (fa.gcount() != fb.gcount() || !std::equal(ba.begin(), ba.begin() + fa.gcount(), bb.begin())) return false;
    }
    return fa.eof() && fb.eof();
}

// A backup is named <hash of the full path>_<file name> in one dated folder: two files of the same name never
// collide, and the path stays short (mirroring the whole original path ran past Windows' 260-character limit).
// The record line keeps the real place, so nothing needs to be read from the name.
fs::path backup_target(const fs::path& backups_root, const fs::path& dest) {
    wchar_t hex[20];
    swprintf_s(hex, L"%016llx", (unsigned long long)std::hash<std::wstring>{}(dest.wstring()));
    return backups_root / (std::wstring(hex) + L"_" + dest.filename().wstring());
}

bool copy_file_over(const fs::path& from, const fs::path& to) {
    std::error_code ec;
    fs::create_directories(to.parent_path(), ec);
    fs::copy_file(from, to, fs::copy_options::overwrite_existing, ec);
    if (ec) logf(L"  ! could not copy %s -> %s (%S)", from.c_str(), to.c_str(), ec.message().c_str());
    return !ec;
}

bool is_keep_if_present(const fs::path& rel) {
    if (rel.has_parent_path()) return false;  // only the profile's top-level settings files
    for (auto* name : cfg::KEEP_IF_PRESENT)
        if (_wcsicmp(rel.c_str(), name) == 0) return true;
    return false;
}

struct Counts {
    int added = 0, replaced = 0, updated = 0, same = 0, kept = 0, failed = 0;
};

// Folders we create get a D line, outermost first, so uninstall can remove them innermost first.
void make_dirs(const fs::path& dir, std::vector<Entry>& rec) {
    std::error_code ec;
    if (fs::exists(dir, ec)) return;
    make_dirs(dir.parent_path(), rec);
    if (fs::create_directory(dir, ec)) rec.push_back({L'D', dir, {}});
}

void install_tree(const fs::path& src_root, const fs::path& dest_root, bool use_keep_list, const fs::path& backups,
                  std::vector<Entry>& rec, Counts& n) {
    std::map<std::wstring, size_t> ours;  // dest -> record index, for files we placed earlier
    for (size_t i = 0; i < rec.size(); ++i)
        if (rec[i].kind == L'A' || rec[i].kind == L'R') ours[rec[i].dest.wstring()] = i;

    std::error_code ec;
    for (auto it = fs::recursive_directory_iterator(src_root, ec); !ec && it != fs::recursive_directory_iterator();
         it.increment(ec)) {
        if (!it->is_regular_file()) continue;
        fs::path rel = fs::relative(it->path(), src_root);
        fs::path dest = dest_root / rel;
        bool exists = fs::exists(dest);
        bool keep = use_keep_list && is_keep_if_present(rel);

        if (ours.count(dest.wstring())) {               // ours already: refresh it, unless the player may have tuned it
            if (keep && exists) { ++n.kept; continue; }
            if (exists && same_bytes(it->path(), dest)) { ++n.same; continue; }
            copy_file_over(it->path(), dest) ? ++n.updated : ++n.failed;
        } else if (exists) {                            // someone else's file: keep a copy, then ours goes in
            if (keep) { ++n.kept; logf(L"  kept the one already there: %s", rel.c_str()); continue; }
            fs::path bk = backup_target(backups, dest);
            if (!copy_file_over(dest, bk)) { ++n.failed; continue; }
            rec.push_back({L'R', dest, bk});
            copy_file_over(it->path(), dest) ? ++n.replaced : ++n.failed;
        } else {
            make_dirs(dest.parent_path(), rec);
            rec.push_back({L'A', dest, {}});
            copy_file_over(it->path(), dest) ? ++n.added : ++n.failed;
        }
    }
}

}  // namespace

bool install_all(const Places& pl) {
    auto rec = load_record(pl);
    fs::path backups = pl.our_dir / cfg::BACKUPS_DIR / now_stamp();
    Counts n;
    std::error_code ec;

    fs::path pkg_profile = pl.launcher_dir / cfg::PKG_PROFILE;
    fs::path pkg_paks = pl.launcher_dir / cfg::PKG_PAKS;
    if (!fs::exists(pkg_profile, ec)) { logf(L"! the package's profile folder is missing: %s", pkg_profile.c_str()); return false; }

    logf(L"Installing the VR profile -> %s", pl.profile_dir.c_str());
    install_tree(pkg_profile, pl.profile_dir, true, backups, rec, n);
    if (fs::exists(pkg_paks, ec)) {
        if (pl.game_dir.empty()) { logf(L"! game folder unknown: the paks were not placed"); ++n.failed; }
        else {
            logf(L"Installing the paks -> %s", (pl.game_dir / cfg::PAKS_MODS_REL).c_str());
            install_tree(pkg_paks, pl.game_dir / cfg::PAKS_MODS_REL, false, backups, rec, n);
        }
    }
    save_record(pl, rec);
    logf(L"  %d added, %d replaced (backed up), %d updated, %d unchanged, %d kept, %d failed", n.added, n.replaced,
         n.updated, n.same, n.kept, n.failed);
    return n.failed == 0;
}

bool reset_vanilla(const Places& pl) {
    // The game keeps the player's button changes here; our button layer is built on the vanilla layout, so the
    // file is backed up and removed before every launch. [hypothesis] the game then starts on its defaults.
    fs::path sav = pl.savegames_dir / cfg::INPUT_REBIND_SAV;
    std::error_code ec;
    if (!fs::exists(sav, ec)) return true;
    auto rec = load_record(pl);
    bool first = std::none_of(rec.begin(), rec.end(), [&](const Entry& e) { return e.kind == L'M' && e.dest == sav; });
    fs::path bk = backup_target(pl.our_dir / cfg::BACKUPS_DIR / now_stamp(), sav);
    if (!copy_file_over(sav, bk)) return false;
    fs::remove(sav, ec);
    if (ec) { logf(L"! could not remove %s", sav.c_str()); return false; }
    if (first) { rec.push_back({L'M', sav, bk}); save_record(pl, rec); }
    logf(L"Controls set back to the game's defaults (the old button file is kept in the backups)");
    return true;
}

bool uninstall_all(const Places& pl) {
    auto rec = load_record(pl);
    if (rec.empty()) { logf(L"Nothing to take out: no install record."); return true; }
    int bad = 0;
    std::error_code ec;
    for (auto it = rec.rbegin(); it != rec.rend(); ++it) {
        switch (it->kind) {
        case L'A': fs::remove(it->dest, ec); if (ec) { ++bad; logf(L"  ! could not remove %s", it->dest.c_str()); } break;
        case L'R':
        case L'M': if (!copy_file_over(it->backup, it->dest)) ++bad; break;
        case L'D': if (fs::is_empty(it->dest, ec)) fs::remove(it->dest, ec); break;
        }
    }
    if (bad == 0) {
        fs::rename(record_path(pl), pl.our_dir / (std::wstring(L"install-record.removed-") + now_stamp() + L".tsv"), ec);
        logf(L"Taken out: %zu record lines undone. Backups are kept in %s", rec.size(), (pl.our_dir / cfg::BACKUPS_DIR).c_str());
    } else {
        logf(L"! %d step(s) failed; the record is kept so uninstall can be run again", bad);
    }
    return bad == 0;
}

void print_status(const Places& pl) {
    auto rec = load_record(pl);
    int a = 0, r = 0, m = 0, d = 0;
    for (auto& e : rec) e.kind == L'A' ? ++a : e.kind == L'R' ? ++r : e.kind == L'M' ? ++m : ++d;
    std::error_code ec;
    logf(L"Game folder:   %s%s", pl.game_dir.empty() ? L"(not found)" : pl.game_dir.c_str(),
         looks_like_game_dir(pl.game_dir) ? L"" : L"  ! the game's exe is not there");
    logf(L"VR profile:    %s", pl.profile_dir.c_str());
    logf(L"UEVR to use:   %s%s", pl.uevr_dir.c_str(),
         fs::exists(pl.uevr_dir / cfg::UEVR_BACKEND, ec) ? L"" : L"  ! UEVRBackend.dll is not there");
    logf(L"Installed:     %d added, %d replaced, %d moved away, %d folders made%s", a, r, m, d,
         pl.test ? L"  (TEST ROOT)" : L"");
}
