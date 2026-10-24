// bridge.h -- the two-way line between the C++ plugin and the Lua pack while both run (the hybrid port).
//   Lua -> C++: scripts/sh2vr_bridge.lua sends "sh2vr_flags" custom events
//               ("hidden=1;animL=0;animR=0;gripL=-;gripR=pistol;mont=3") with the answers the pack's modules still
//               decide (UI open, montage playing, which grip pose each hand holds, montages ended so far).
//   C++ -> Lua: the plugin sends Lua events naming the objects it made (the IK arm mesh), so the pack's modules
//               that hang things on them (attachments, melee, accessories) keep working until they are ported.
#pragma once

#include <string>
#include <string_view>

namespace sh2vr::bridge {

void on_custom_event(const char* name, const char* data);   // from the plugin's on_custom_event
bool heard();                                               // true once the Lua side has sent anything
bool hands_hidden();
bool anim_left();
bool anim_right();
const std::string& grip_left();    // what the left hand holds, for the finger poses: "-" nothing, "+" the default
const std::string& grip_right();   //   grip, else the grip animation's name ("pistol"); "" until the Lua has spoken
int montage_count();               // how many hand montages have ended: a change means "re-pose the fingers"

void tell_lua(std::string_view event, std::string_view data);   // dispatch_lua_event

} // namespace sh2vr::bridge
