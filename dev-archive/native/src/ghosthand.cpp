// ghosthand.cpp -- see ghosthand.h. The writer is Visceral RE8's (same line format); the samples are Silent Hill 2's.
#include "ghosthand.h"

#include <windows.h>

#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <string>

#include "gun.h"
#include "ik.h"
#include "ikmath.h"
#include "input.h"
#include "rack.h"
#include "reload.h"
#include "settings.h"

namespace sh2vr::ghosthand {

using namespace sh2vr::ikmath;
using gun::Rot;
using gun::Vec3;

namespace {
FILE* g_f = nullptr;
std::string g_buf;
std::chrono::steady_clock::time_point g_t0, g_last_flush;
bool g_need_sent = false;

// world -> room: the two controllers are known in both spaces (the game's components in the world, UEVR's poses in the
// room), so the room is the world turned about the up axis by the yaw between them, scaled 0.01, from the left controller
struct Map { bool ok = false; Vec3 world_l; float room_l[3]{}; double yaw = 0.0; } g_map;

long long now_us() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - g_t0).count(); }

void flush() {
    if (g_f != nullptr && !g_buf.empty()) { std::fwrite(g_buf.data(), 1, g_buf.size(), g_f); std::fflush(g_f); g_buf.clear(); }
    g_last_flush = std::chrono::steady_clock::now();
}

void put(const char* fmt, ...) {
    if (g_f == nullptr) return;
    char line[256];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    g_buf += line;
    if (g_buf.size() > 64 * 1024) flush();
}

void vr_on() {
    if (g_f != nullptr) return;
    char base[MAX_PATH]{};
    const DWORD got = GetEnvironmentVariableA("LOCALAPPDATA", base, MAX_PATH);
    std::string dir = std::string(got > 0 && got < MAX_PATH ? base : ".") + "\\BeG0nE";
    CreateDirectoryA(dir.c_str(), nullptr);
    dir += "\\ghost-hand";
    CreateDirectoryA(dir.c_str(), nullptr);
    char stamp[32];
    const std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &t);
    std::strftime(stamp, sizeof stamp, "%Y%m%d-%H%M%S", &tm);
    const std::string path = dir + "\\silent-hill-2-remake-vr_" + stamp + ".ghl";
    if (fopen_s(&g_f, path.c_str(), "wb") != 0) { g_f = nullptr; LOGW("%s ghost hand: could not open %s", TAG, path.c_str()); return; }
    g_t0 = std::chrono::steady_clock::now();
    g_need_sent = false;
    put("#ghosthand 1 game=silent-hill-2-remake-vr runtime=openxr pid=%lu\n", GetCurrentProcessId());   // pid: the Lanes rider records this program's window
    flush();
    LOGI("%s ghost hand: recording to %s", TAG, path.c_str());
}

void vr_off() {
    if (g_f == nullptr) return;
    put("#end\n");
    flush();
    std::fclose(g_f);
    g_f = nullptr;
    LOGI("%s ghost hand: recording closed", TAG);
}

bool room_pose(UEVR_TrackedDeviceIndex idx, float out[3]) {
    const auto p = API::VR::get_pose(idx);
    out[0] = p.position.x; out[1] = p.position.y; out[2] = p.position.z;
    return !(p.position.x == 0.0f && p.position.y == 0.0f && p.position.z == 0.0f);
}

void update_map() {
    Vec3 wl, wr;
    Rot r;
    float rl[3], rr[3];
    if (!gun::controller(controllers::LEFT, wl, r) || !gun::controller(controllers::RIGHT, wr, r)) { g_map.ok = false; return; }
    if (!room_pose(API::VR::get_left_controller_index(), rl) || !room_pose(API::VR::get_right_controller_index(), rr)) { g_map.ok = false; return; }
    const double wx = wr.x - wl.x, wy = wr.y - wl.y;                 // world: X forward, Y right (Z up)
    const double rx = rr[0] - rl[0], rz = rr[2] - rl[2];             // room: X right, -Z forward (Y up)
    const bool wide = std::sqrt(wx * wx + wy * wy) > settings::GHOST_MAP_MIN_CM;
    if (wide) g_map.yaw = std::atan2(rx, -rz) - std::atan2(wy, wx);  // the turn that takes the world's line between the hands onto the room's
    else if (!g_map.ok) return;                                      // hands together and no earlier yaw: nothing to map with yet
    g_map.world_l = wl;
    g_map.room_l[0] = rl[0]; g_map.room_l[1] = rl[1]; g_map.room_l[2] = rl[2];
    g_map.ok = true;
}

bool world_to_room(const Vec3& w, float out[3]) {
    if (!g_map.ok) return false;
    const double dx = (w.x - g_map.world_l.x) * 0.01, dy = (w.y - g_map.world_l.y) * 0.01, dz = (w.z - g_map.world_l.z) * 0.01;
    const double a = std::atan2(dy, dx) + g_map.yaw, len = std::sqrt(dx * dx + dy * dy);   // forward/right in the world -> the room's horizontal plane
    out[0] = g_map.room_l[0] + (float)(len * std::sin(a));
    out[1] = g_map.room_l[1] + (float)dz;
    out[2] = g_map.room_l[2] - (float)(len * std::cos(a));
    return true;
}

void real(const char* side, UEVR_TrackedDeviceIndex idx) {
    float p[3];
    const bool tracked = room_pose(idx, p);
    put("%lld R %s %d %.4f %.4f %.4f\n", now_us(), side, tracked ? 1 : 0, tracked ? p[0] : 0.0f, tracked ? p[1] : 0.0f, tracked ? p[2] : 0.0f);
}

void world_line(const char* kind, const char* name, bool have, const Vec3& w) {
    float r[3];
    if (have && world_to_room(w, r)) put("%lld %s %s %.4f %.4f %.4f\n", now_us(), kind, name, r[0], r[1], r[2]);
}
} // namespace

void event(const char* gesture, const char* what) { put("%lld E %s %s\n", now_us(), gesture, what); }
void value(const char* name, float v) { put("%lld V %s %.5f\n", now_us(), name, v); }

void tick() {
    if (!settings::GHOST_HAND_ON) return;
    if (!gun::hmd_active()) { vr_off(); return; }
    vr_on();
    if (g_f == nullptr) return;
    if (!g_need_sent) { value("rack_need", (float)(settings::SLIDE_TRAVEL_CM * 0.01)); value("mag_need", (float)(settings::MAG_DOCK_CM * 0.01)); g_need_sent = true; }
    update_map();
    real("L", API::VR::get_left_controller_index());
    real("R", API::VR::get_right_controller_index());
    Vec3 hl, hr;
    const bool have_l = ik::hand_world(controllers::LEFT, hl), have_r = ik::hand_world(controllers::RIGHT, hr);
    world_line("G", "L", have_l, hl);
    world_line("G", "R", have_r, hr);
    Vec3 slide;
    API::UObject* w = gun::equipped();
    world_line("P", "slide", gun::is_pistol(w) && gun::socket_loc(gun::mesh(w), settings::SLIDE_SOCKET, slide), slide);
    put("%lld B LG %d\n", now_us(), input::held(XINPUT_GAMEPAD_LEFT_SHOULDER) ? 1 : 0);
    put("%lld B LT %d\n", now_us(), input::left_trigger() ? 1 : 0);
    put("%lld B RB %d\n", now_us(), input::held(XINPUT_GAMEPAD_B) ? 1 : 0);
    put("%lld F\n", now_us());
    if (std::chrono::steady_clock::now() - g_last_flush > std::chrono::seconds(1)) flush();
}

void shutdown() { vr_off(); }

} // namespace sh2vr::ghosthand
