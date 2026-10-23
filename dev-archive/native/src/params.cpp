// params.cpp -- see params.h
#include "params.h"

#include <windows.h>

#include <map>

#include "common.h"
#include "settings.h"

namespace sh2vr::params {

namespace {
struct Entry {
    json::Value value;
    bool loaded = false;
    FILETIME written{};
};
std::map<std::string, Entry> g_files;

std::wstring path_of(const std::string& name) {
    return appdata_dir() + L"\\" + settings::PROFILE_DIR + L"\\" + settings::PARAMS_SUBDIR + L"\\" + std::wstring(name.begin(), name.end());
}

bool write_time(const std::wstring& path, FILETIME& out) {
    WIN32_FILE_ATTRIBUTE_DATA a{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &a)) return false;
    out = a.ftLastWriteTime;
    return true;
}

void load_one(const std::string& name, bool say) {
    Entry& e = g_files[name];
    const std::wstring p = path_of(name);
    FILETIME t{};
    if (!write_time(p, t)) { e.loaded = false; if (say) LOGW("%s params: %s not found", TAG, name.c_str()); return; }
    json::Value v;
    if (!json::load_file(p, v) || v.kind != json::Value::OBJ) { e.loaded = false; LOGW("%s params: %s could not be read", TAG, name.c_str()); return; }
    e.value = std::move(v);
    e.loaded = true;
    e.written = t;
    if (say) LOGI("%s params: %s read, %zu top-level keys", TAG, name.c_str(), e.value.obj.size());
}
} // namespace

void load_all() {
    for (const auto& f : FILES) load_one(f, true);
    LOGI("%s params: %d of %zu files read", TAG, loaded_count(), FILES.size());
}

void watch() {
    for (auto& [name, e] : g_files) {
        FILETIME t{};
        if (!write_time(path_of(name), t)) continue;
        if (e.loaded && CompareFileTime(&t, &e.written) == 0) continue;
        load_one(name, false);
        LOGI("%s params: %s changed on disk, re-read", TAG, name.c_str());
    }
}

const json::Value* file(const std::string& name) {
    auto it = g_files.find(name);
    return it != g_files.end() && it->second.loaded ? &it->second.value : nullptr;
}

int loaded_count() {
    int n = 0;
    for (const auto& [name, e] : g_files) n += e.loaded ? 1 : 0;
    return n;
}

} // namespace sh2vr::params
