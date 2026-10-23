// bridge.cpp -- see bridge.h
#include "bridge.h"

#include <cstring>
#include <string>

#include "common.h"

namespace sh2vr::bridge {

namespace {
bool g_heard = false, g_hidden = false, g_anim_l = false, g_anim_r = false;

bool value_of(const std::string& data, const char* key) {   // "hidden=1;animL=0;animR=0" -> the digit after key=
    const std::string k = std::string(key) + "=";
    const size_t at = data.find(k);
    return at != std::string::npos && at + k.size() < data.size() && data[at + k.size()] == '1';
}
} // namespace

void on_custom_event(const char* name, const char* data) {
    if (name == nullptr || data == nullptr || std::strcmp(name, "sh2vr_flags") != 0) return;
    const std::string d(data);
    g_hidden = value_of(d, "hidden");
    g_anim_l = value_of(d, "animL");
    g_anim_r = value_of(d, "animR");
    if (!g_heard) { g_heard = true; LOGI("%s bridge: the Lua side is talking (%s)", TAG, data); }
}

bool heard() { return g_heard; }
bool hands_hidden() { return g_hidden; }
bool anim_left() { return g_anim_l; }
bool anim_right() { return g_anim_r; }

void tell_lua(std::string_view event, std::string_view data) {
    auto& api = API::get();
    if (api) api->dispatch_lua_event(event, data);
}

} // namespace sh2vr::bridge
