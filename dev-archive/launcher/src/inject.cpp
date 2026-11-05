#include "inject.h"

#include "settings.h"

#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>

#include <sstream>

namespace {

struct Handle {
    HANDLE h = nullptr;
    explicit Handle(HANDLE x) : h(x) {}
    ~Handle() { if (h && h != INVALID_HANDLE_VALUE) CloseHandle(h); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    explicit operator bool() const { return h && h != INVALID_HANDLE_VALUE; }
};

// Calls `fn(arg)` inside the game in a new thread; returns the thread's exit code (low 32 bits of the result).
bool remote_call(HANDLE proc, void* fn, void* arg, DWORD& code) {
    Handle th(CreateRemoteThread(proc, nullptr, 0, (LPTHREAD_START_ROUTINE)fn, arg, 0, nullptr));
    if (!th) return false;
    if (WaitForSingleObject(th.h, cfg::REMOTE_THREAD_MS) != WAIT_OBJECT_0) return false;
    return GetExitCodeThread(th.h, &code) != 0;
}

// LoadLibraryW(full path) inside the game. kernel32 sits at the same address in every process of a session.
bool load_into(HANDLE proc, const fs::path& dll) {
    std::wstring path = dll.wstring();
    SIZE_T bytes = (path.size() + 1) * sizeof(wchar_t);
    void* mem = VirtualAllocEx(proc, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) return false;
    bool ok = WriteProcessMemory(proc, mem, path.c_str(), bytes, nullptr) != 0;
    DWORD code = 0;
    if (ok) {
        void* load = (void*)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
        ok = remote_call(proc, load, mem, code) && code != 0;
    }
    VirtualFreeEx(proc, mem, 0, MEM_RELEASE);
    return ok;
}

uintptr_t remote_module_base(uint32_t pid, const wchar_t* name) {
    Handle snap(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
    if (!snap) return 0;
    MODULEENTRY32W me{sizeof(me)};
    for (BOOL ok = Module32FirstW(snap.h, &me); ok; ok = Module32NextW(snap.h, &me))
        if (_wcsicmp(me.szModule, name) == 0) return (uintptr_t)me.modBaseAddr;
    return 0;
}

// The nullifier's "nullify" export, found by loading the same DLL here and applying its offset there.
bool call_nullifier(HANDLE proc, uint32_t pid, const fs::path& dll) {
    HMODULE here = LoadLibraryExW(dll.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!here) return false;
    auto fn = (uintptr_t)GetProcAddress(here, cfg::NULLIFIER_EXPORT);
    uintptr_t offset = fn ? fn - (uintptr_t)here : 0;
    FreeLibrary(here);
    uintptr_t there = remote_module_base(pid, cfg::UEVR_NULLIFIER);
    DWORD code = 0;
    return offset && there && remote_call(proc, (void*)(there + offset), nullptr, code);
}

// UEVR reads which runtime to start from the profile's config.txt; the frontend writes it just before the backend.
void set_runtime_line(const Places& pl, const wchar_t* runtime) {
    fs::path cfgfile = pl.profile_dir / L"config.txt";
    std::wistringstream in(read_text_file(cfgfile));
    std::wstring line, out;
    bool found = false;
    const std::wstring key = std::wstring(cfg::RUNTIME_KEY) + L"=";
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        if (line.rfind(key, 0) == 0) { line = key + runtime; found = true; }
        out += line + L"\n";
    }
    if (!found) out += key + runtime + L"\n";
    std::error_code ec;
    fs::create_directories(pl.profile_dir, ec);
    if (!write_text_file(cfgfile, out)) logf(L"! could not write %s", cfgfile.c_str());
}

struct WindowFind {
    DWORD pid;
    bool found;
};

BOOL CALLBACK window_cb(HWND w, LPARAM lp) {
    auto* f = (WindowFind*)lp;
    DWORD pid = 0;
    GetWindowThreadProcessId(w, &pid);
    if (pid == f->pid && IsWindowVisible(w) && GetWindow(w, GW_OWNER) == nullptr && GetWindowTextLengthW(w) > 0) {
        f->found = true;
        return FALSE;
    }
    return TRUE;
}

}  // namespace

uint32_t find_game_pid() {
    Handle snap(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!snap) return 0;
    PROCESSENTRY32W pe{sizeof(pe)};
    for (BOOL ok = Process32FirstW(snap.h, &pe); ok; ok = Process32NextW(snap.h, &pe))
        if (_wcsicmp(pe.szExeFile, cfg::GAME_EXE) == 0) return pe.th32ProcessID;
    return 0;
}

bool launch_through_steam() {
    std::wstring url = std::wstring(L"steam://rungameid/") + cfg::STEAM_APP_ID;
    return (INT_PTR)ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL) > 32;
}

uint32_t wait_for_game(uint32_t timeout_ms) {
    for (uint32_t t = 0; t < timeout_ms; t += cfg::POLL_MS) {
        if (uint32_t pid = find_game_pid()) return pid;
        Sleep(cfg::POLL_MS);
    }
    return 0;
}

bool wait_for_window(uint32_t pid, uint32_t timeout_ms) {
    for (uint32_t t = 0; t < timeout_ms; t += cfg::POLL_MS) {
        WindowFind f{pid, false};
        EnumWindows(window_cb, (LPARAM)&f);
        if (f.found) return true;
        if (find_game_pid() != pid) return false;  // the game closed while we waited
        Sleep(cfg::POLL_MS);
    }
    return false;
}

bool module_loaded(uint32_t pid, const wchar_t* name) { return remote_module_base(pid, name) != 0; }

bool inject_uevr(uint32_t pid, const Places& pl, const InjectOptions& o) {
    const wchar_t* runtime = o.openvr ? cfg::RUNTIME_OPENVR : cfg::RUNTIME_OPENXR;
    std::error_code ec;
    for (const wchar_t* f : {runtime, cfg::UEVR_BACKEND})
        if (!fs::exists(pl.uevr_dir / f, ec)) { logf(L"! UEVR file missing: %s", (pl.uevr_dir / f).c_str()); return false; }
    if (module_loaded(pid, cfg::UEVR_BACKEND)) { logf(L"UEVR is already loaded in the game."); return true; }

    Handle proc(OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                                PROCESS_VM_WRITE | PROCESS_VM_READ,
                            FALSE, pid));
    if (!proc) { logf(L"! could not open the game (error %lu)", GetLastError()); return false; }

    if (o.nullify) {
        fs::path nul = pl.uevr_dir / cfg::UEVR_NULLIFIER;
        bool ok = load_into(proc.h, nul) && call_nullifier(proc.h, pid, nul);
        logf(ok ? L"VR plugin nullifier ran." : L"! the VR plugin nullifier did not run (carrying on)");
    }
    if (!load_into(proc.h, pl.uevr_dir / runtime)) { logf(L"! could not load %s into the game", runtime); return false; }
    set_runtime_line(pl, runtime);
    if (!load_into(proc.h, pl.uevr_dir / cfg::UEVR_BACKEND)) { logf(L"! could not load UEVR into the game"); return false; }
    logf(L"UEVR loaded (%s).", runtime);
    return true;
}
