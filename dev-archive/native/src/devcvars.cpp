// devcvars.cpp -- see devcvars.h.
#include "devcvars.h"

#include <windows.h>

#include <cstdlib>
#include <cwchar>
#include <fstream>
#include <string>

#include "common.h"
#include "settings.h"
#include "uevr/API.hpp"

namespace sh2vr::devcvars {

namespace {
constexpr float CHECK_S = 1.0f;
constexpr const wchar_t* FILE_NAME = L"sh2vr_devcvars.txt";
float g_since = CHECK_S;
FILETIME g_seen{};

std::wstring path() { return appdata_dir() + L"\\" + settings::PROFILE_DIR + L"\\" + FILE_NAME; }

bool changed(const std::wstring& p) {
    WIN32_FILE_ATTRIBUTE_DATA a{};
    if (!GetFileAttributesExW(p.c_str(), GetFileExInfoStandard, &a)) return false;   // no file: nothing to do
    if (CompareFileTime(&a.ftLastWriteTime, &g_seen) == 0) return false;
    g_seen = a.ftLastWriteTime;
    return true;
}
} // namespace

void tick(float delta) {
    g_since += delta;
    if (g_since < CHECK_S) return;
    g_since = 0.0f;
    const std::wstring p = path();
    if (!changed(p)) return;
    auto* cm = API::get()->get_console_manager();
    if (cm == nullptr) return;
    std::wifstream f(p.c_str());
    std::wstring line;
    int applied = 0, missing = 0;
    while (std::getline(f, line)) {
        const size_t hash = line.find(L'#');
        if (hash != std::wstring::npos) line.erase(hash);
        const size_t a = line.find_first_not_of(L" \t\r");
        if (a == std::wstring::npos) continue;
        const size_t b = line.find_first_of(L" \t=", a);
        if (b == std::wstring::npos) continue;
        const std::wstring name = line.substr(a, b - a);
        const size_t c = line.find_first_not_of(L" \t=", b);
        if (c == std::wstring::npos) continue;
        const float value = (float)std::wcstod(line.c_str() + c, nullptr);
        auto* v = cm->find_variable(name);
        if (v == nullptr) {
            ++missing;
            LOGW("%s devcvars: %ls is not in this build", TAG, name.c_str());
            continue;
        }
        const float before = v->get_float();
        v->set(value);
        ++applied;
        LOGI("%s devcvars: %ls %g -> %g", TAG, name.c_str(), before, v->get_float());
    }
    LOGI("%s devcvars: file read, %d applied, %d not found", TAG, applied, missing);
}

} // namespace sh2vr::devcvars
