// bridge.cpp -- see bridge.h
#include "bridge.h"

#include <cstdlib>
#include <cstring>
#include <string>

#include "common.h"

namespace sh2vr::bridge {

namespace {
bool g_heard = false, g_hidden = false, g_anim_l = false, g_anim_r = false;
std::string g_grip_l, g_grip_r;
int g_montages = 0;

bool value_of(const std::string& data, const char* key) {   // "hidden=1;animL=0;animR=0" -> the digit after key=
    const std::string k = std::string(key) + "=";
    const size_t at = data.find(k);
    return at != std::string::npos && at + k.size() < data.size() && data[at + k.size()] == '1';
}
std::string text_of(const std::string& data, const char* key, const std::string& fallback) {   // the text after key= up to ';'
    const std::string k = std::string(key) + "=";
    size_t at = data.find(k);
    if (at == std::string::npos) return fallback;
    at += k.size();
    const size_t end = data.find(';', at);
    return data.substr(at, end == std::string::npos ? std::string::npos : end - at);
}
} // namespace

void on_custom_event(const char* name, const char* data) {
    if (name == nullptr || data == nullptr || std::strcmp(name, "sh2vr_flags") != 0) return;
    const std::string d(data);
    g_hidden = value_of(d, "hidden");
    g_anim_l = value_of(d, "animL");
    g_anim_r = value_of(d, "animR");
    g_grip_l = text_of(d, "gripL", g_grip_l);
    g_grip_r = text_of(d, "gripR", g_grip_r);
    g_montages = std::atoi(text_of(d, "mont", std::to_string(g_montages)).c_str());
    if (!g_heard) { g_heard = true; LOGI("%s bridge: the Lua side is talking (%s)", TAG, data); }
}

bool heard() { return g_heard; }
bool hands_hidden() { return g_hidden; }
bool anim_left() { return g_anim_l; }
bool anim_right() { return g_anim_r; }
const std::string& grip_left() { return g_grip_l; }
const std::string& grip_right() { return g_grip_r; }
int montage_count() { return g_montages; }

void tell_lua(std::string_view event, std::string_view data) {
    auto& api = API::get();
    if (api) api->dispatch_lua_event(event, data);
}

} // namespace sh2vr::bridge
