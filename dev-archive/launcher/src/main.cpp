// SH2VR.exe: one program for the Silent Hill 2 VR mod. With no command it installs or updates the mod, puts the
// controls back to the game's defaults, starts the game through Steam and loads UEVR into it, then exits.
//
//   SH2VR.exe                     play (the default)
//   SH2VR.exe install             install / update only
//   SH2VR.exe uninstall           take everything out again, from the install record
//   SH2VR.exe status              where everything is, what is installed
//   SH2VR.exe attach              load UEVR into a game that is already running
// Options: --game <folder>  (remembered)   --uevr <folder> (another UEVR build, e.g. PureDark's AFW)
//          --openvr   --nullify   --delay <ms>   --test-root <folder> (every destination goes there; nothing real
//          is touched and nothing is launched)
#include "common.h"
#include "inject.h"
#include "install.h"
#include "settings.h"
#include "steam.h"

#include <windows.h>

#include <cwchar>
#include <string>

namespace {

struct Args {
    std::wstring command = L"play";
    fs::path game, uevr, test_root;
    InjectOptions inj{false, false, cfg::INJECT_DELAY_MS};
    bool bad = false;
};

Args parse(int argc, wchar_t** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::wstring s = argv[i];
        auto next = [&]() -> std::wstring { return i + 1 < argc ? argv[++i] : (a.bad = true, L""); };
        if (s == L"--game") a.game = next();
        else if (s == L"--uevr") a.uevr = next();
        else if (s == L"--test-root") a.test_root = next();
        else if (s == L"--openvr") a.inj.openvr = true;
        else if (s == L"--nullify") a.inj.nullify = true;
        else if (s == L"--delay") a.inj.delay_ms = (uint32_t)wcstoul(next().c_str(), nullptr, 10);
        else if (s.rfind(L"--", 0) == 0) a.bad = true;
        else a.command = s;
    }
    return a;
}

Places resolve(const Args& a) {
    Places pl;
    pl.launcher_dir = exe_dir();
    pl.uevr_dir = a.uevr.empty() ? pl.launcher_dir / cfg::PKG_UEVR : a.uevr;
    if (!a.test_root.empty()) {
        pl.test = true;
        fs::path r = fs::absolute(a.test_root);
        pl.profile_dir = r / L"Roaming" / L"UnrealVRMod" / cfg::PROFILE_NAME;
        pl.savegames_dir = r / L"Local" / cfg::SAVEGAMES_REL;
        pl.our_dir = r / L"Local" / cfg::OUR_DIR;
        pl.game_dir = a.game.empty() ? r / L"Game" : a.game;
        return pl;
    }
    pl.profile_dir = known_folder_roaming() / L"UnrealVRMod" / cfg::PROFILE_NAME;
    pl.savegames_dir = known_folder_local() / cfg::SAVEGAMES_REL;
    pl.our_dir = known_folder_local() / cfg::OUR_DIR;

    fs::path remembered = pl.our_dir / cfg::GAME_DIR_FILE;
    if (!a.game.empty()) {
        pl.game_dir = a.game;
        std::error_code ec;
        fs::create_directories(pl.our_dir, ec);
        write_text_file(remembered, pl.game_dir.wstring());
    } else {
        std::wstring r = read_text_file(remembered);
        while (!r.empty() && (r.back() == L'\n' || r.back() == L'\r')) r.pop_back();
        pl.game_dir = !r.empty() ? fs::path(r) : find_game_dir();
    }
    return pl;
}

int play(const Places& pl, const InjectOptions& inj, bool launch) {
    if (!pl.test && !looks_like_game_dir(pl.game_dir)) {
        logf(L"! Silent Hill 2 was not found. Run once with --game \"<the game's folder>\".");
        return 2;
    }
    uint32_t pid = find_game_pid();
    if (pid && module_loaded(pid, cfg::UEVR_BACKEND)) { logf(L"The game is already running in VR."); return 0; }
    if (!pid) {   // only while the game is closed: the profile and the button file are in use once it runs
        if (!install_all(pl)) logf(L"! some files were not placed (see above); trying anyway");
        reset_vanilla(pl);
    }
    if (pl.test) { logf(L"(test root: not launching anything)"); return 0; }

    if (!pid) {
        if (!launch) { logf(L"! the game is not running."); return 3; }
        logf(L"Starting Silent Hill 2 through Steam...");
        if (!launch_through_steam()) { logf(L"! Steam did not take the start request."); return 4; }
        pid = wait_for_game(cfg::WAIT_PROCESS_MS);
        if (!pid) { logf(L"! the game did not start within %u s.", cfg::WAIT_PROCESS_MS / 1000); return 5; }
    }
    logf(L"Waiting for the game's window...");
    if (!wait_for_window(pid, cfg::WAIT_WINDOW_MS)) { logf(L"! the game's window never showed."); return 6; }
    Sleep(inj.delay_ms);
    return inject_uevr(pid, pl, inj) ? 0 : 7;
}

// Started by a double-click, the console window is ours alone and closes the moment we exit: on a failure keep it
// open until Enter, so the player can read why; on success leave it up a few seconds.
int finish(int rc) {
    DWORD ids[4];
    if (GetConsoleProcessList(ids, 4) == 1) {
        if (rc != 0) {
            wprintf(L"\nSomething went wrong (see above). Press Enter to close this window.\n");
            (void)getwchar();
        } else {
            Sleep(cfg::CLOSE_AFTER_OK_MS);
        }
    }
    return rc;
}

int run(int argc, wchar_t** argv);

}  // namespace

int wmain(int argc, wchar_t** argv) { return finish(run(argc, argv)); }

namespace {

int run(int argc, wchar_t** argv) {
    Args a = parse(argc, argv);
    if (a.bad) { wprintf(L"Unknown or incomplete option. See the top of main.cpp / the readme.\n"); return 1; }
    Places pl = resolve(a);
    log_open(pl.our_dir / cfg::LOG_FILE);
    logf(L"SH2VR launcher, command: %s%s", a.command.c_str(), pl.test ? L" (test root)" : L"");

    if (a.command == L"play") return play(pl, a.inj, true);
    if (a.command == L"attach") return play(pl, a.inj, false);
    if (a.command == L"status") { print_status(pl); return 0; }
    if (a.command == L"install" || a.command == L"uninstall") {
        if (find_game_pid() && !pl.test) { logf(L"! close the game first."); return 3; }
        if (a.command == L"uninstall") return uninstall_all(pl) ? 0 : 8;
        bool ok = install_all(pl);
        return reset_vanilla(pl) && ok ? 0 : 8;
    }
    logf(L"! unknown command: %s", a.command.c_str());
    return 1;
}

}  // namespace
