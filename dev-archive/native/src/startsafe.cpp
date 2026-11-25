// startsafe.cpp -- see startsafe.h.
#include "startsafe.h"

#include <windows.h>

#include <string>
#include <vector>

#include "common.h"
#include "settings.h"
#include "startsafe_text.h"
#include "uevr/API.hpp"

namespace sh2vr::startsafe {

namespace {
std::vector<startsafe_text::Change> g_changes;
std::string g_problem;   // why the file could not be read or written ("" = fine)

std::wstring config_path() {
    wchar_t buf[MAX_PATH];
    const DWORD n = GetEnvironmentVariableW(L"APPDATA", buf, MAX_PATH);   // kernel32 only: this runs under the loader lock
    if (n == 0 || n >= MAX_PATH) return L"";
    return std::wstring(buf) + L"\\" + settings::PROFILE_DIR + L"\\config.txt";
}

bool read_all(const std::wstring& path, std::string& out) {
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    bool ok = GetFileSizeEx(f, &size) && size.QuadPart < (1 << 20);
    if (ok) {
        out.resize(static_cast<size_t>(size.QuadPart));
        DWORD got = 0;
        ok = out.empty() || (ReadFile(f, out.data(), static_cast<DWORD>(out.size()), &got, nullptr) && got == out.size());
    }
    CloseHandle(f);
    return ok;
}

bool write_all(const std::wstring& path, const std::string& text) {
    const std::wstring tmp = path + L".sh2vr-tmp";
    HANDLE f = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    DWORD put = 0;
    const bool ok = WriteFile(f, text.data(), static_cast<DWORD>(text.size()), &put, nullptr) && put == text.size();
    CloseHandle(f);
    if (!ok) { DeleteFileW(tmp.c_str()); return false; }
    return MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;   // whole or not at all
}

void fix_the_file() {
    if (!settings::START_SAFE_ON) return;
    const std::wstring path = config_path();
    std::string text;
    if (path.empty() || !read_all(path, text)) { g_problem = "config.txt could not be read (a first start has none yet)"; return; }
    std::vector<std::pair<std::string, std::string>> wanted;
    for (const auto& kv : settings::START_SAFE) wanted.push_back({kv[0], kv[1]});
    const std::string fixed = startsafe_text::set(text, wanted, g_changes);
    if (!g_changes.empty() && !write_all(path, fixed)) g_problem = "config.txt could not be written: the values stay as they were";
}

// Runs while UEVR loads this DLL (its Framework constructor), before UEVR first reads config.txt.
struct AtLoad {
    AtLoad() { fix_the_file(); }
} g_at_load;
} // namespace

void log_what_was_done() {
    if (!settings::START_SAFE_ON) return;
    if (!g_problem.empty()) LOGW("%s startsafe: %s", TAG, g_problem.c_str());
    for (const auto& c : g_changes)
        LOGW("%s startsafe: config.txt had %s=%s (saved by the last run); put back to %s before UEVR read it", TAG,
             c.key.c_str(), c.was.c_str(), c.now.c_str());
    if (g_changes.empty() && g_problem.empty()) LOGI("%s startsafe: config.txt was already safe", TAG);
    for (const auto& kv : settings::START_SAFE)
        LOGI("%s startsafe: %s is %s now", TAG, kv[0], API::VR::get_mod_value<std::string>(kv[0]).c_str());
}

} // namespace sh2vr::startsafe
