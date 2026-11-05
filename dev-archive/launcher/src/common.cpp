#include "common.h"

#include <windows.h>
#include <shlobj.h>

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>

static FILE* g_log = nullptr;

void log_open(const fs::path& file) {
    std::error_code ec;
    fs::create_directories(file.parent_path(), ec);
    _wfopen_s(&g_log, file.c_str(), L"a, ccs=UTF-8");
    if (g_log) fwprintf(g_log, L"\n=== %s ===\n", now_stamp().c_str());
}

void logf(const wchar_t* fmt, ...) {
    wchar_t buf[2048];
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, ap);
    va_end(ap);
    wprintf(L"%s\n", buf);
    if (g_log) { fwprintf(g_log, L"%s\n", buf); fflush(g_log); }
}

std::wstring widen(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

std::string narrow(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring read_text_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return {};
    std::stringstream ss;
    ss << f.rdbuf();
    std::string s = ss.str();
    if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF)
        s.erase(0, 3);
    return widen(s);
}

bool write_text_file(const fs::path& p, const std::wstring& text) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    std::string s = narrow(text);
    f.write(s.data(), (std::streamsize)s.size());
    return (bool)f;
}

std::wstring now_stamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &t);
    wchar_t buf[32];
    wcsftime(buf, _countof(buf), L"%Y-%m-%d_%H%M%S", &tm);
    return buf;
}

static fs::path known_folder(REFKNOWNFOLDERID id) {
    PWSTR p = nullptr;
    fs::path out;
    if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &p))) out = p;
    CoTaskMemFree(p);
    return out;
}

fs::path known_folder_roaming() { return known_folder(FOLDERID_RoamingAppData); }
fs::path known_folder_local() { return known_folder(FOLDERID_LocalAppData); }

fs::path exe_dir() {
    wchar_t buf[MAX_PATH * 2];
    DWORD n = GetModuleFileNameW(nullptr, buf, _countof(buf));
    return fs::path(std::wstring(buf, n)).parent_path();
}
