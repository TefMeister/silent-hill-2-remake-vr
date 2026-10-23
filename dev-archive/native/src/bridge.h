// bridge.h -- the two-way line between the C++ plugin and the Lua pack while both run (the hybrid port).
//   Lua -> C++: scripts/sh2vr_bridge.lua sends "sh2vr_flags" custom events ("hidden=1;animL=0;animR=0") with the
//               answers the pack's modules still decide (UI open, montage playing).
//   C++ -> Lua: the plugin sends Lua events naming the objects it made (the IK arm mesh), so the pack's modules
//               that hang things on them (attachments, melee, accessories) keep working until they are ported.
#pragma once

#include <string_view>

namespace sh2vr::bridge {

void on_custom_event(const char* name, const char* data);   // from the plugin's on_custom_event
bool heard();                                               // true once the Lua side has sent anything
bool hands_hidden();
bool anim_left();
bool anim_right();

void tell_lua(std::string_view event, std::string_view data);   // dispatch_lua_event

} // namespace sh2vr::bridge
