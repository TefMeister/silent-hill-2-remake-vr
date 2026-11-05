// frontgrip.cpp -- see frontgrip.h. [compile-verified 2026-11-05] until it has run.
//
// Read live 2026-11-05 (the pretend headset, the shotgun out, the game's aim button held through the UEVR MCP):
//   - the weapon actor's RootComponent stays attached to the hidden body at hand_r_socket; the visible Mesh is on OUR
//     arm mesh's hand_r_bn with the pack's offset, and the two sat at the same world transform, so the game's own
//     hand-on-gun relation measured against the root holds against the visible mesh too `[measured 2026-11-05, n=1]`;
//   - aiming, the body's left hand sat 28.8 cm up the shotgun (barrel +Y), 7 cm to the side, 4 cm under; not aiming,
//     26 cm up but turned 44 degrees differently (the loose carry).
//   - CORRECTED the same hour: the game's left-hand IK puts the hand bone exactly on the gun's own socket Holder_L (the
//     shotgun: 5.7 26.1 -1.4 cm from the gun's frame, the body's hand 5.8 26.2 -1.5, the same turn to 0.001)
//     `[measured 2026-11-05, n=2]`; Holder_L sits on the pump (bone_forearm). The MCP's held aim never put the game in
//     its aim (IsInRangedAim false), and the readings taken then wandered 20 cm: so the grip is the socket, read live,
//     and the capture only keeps fingers taken while the body's hand is ON the socket.
#include "frontgrip.h"

#include <windows.h>

#include <cmath>
#include <cstdio>
#include <map>
#include <vector>

#include "animation.h"
#include "common.h"
#include "fingers.h"
#include "gun.h"
#include "ik.h"
#include "params.h"
#include "settings.h"

namespace sh2vr::frontgrip {

namespace {
using ue::Quat;
using ue::Rot;
using ue::Transform;
using ue::Vec3;

constexpr int RTS_WORLD = 0, RTS_PARENT_BONE = 3;   // ERelativeTransformSpace

Quat qmul(Quat a, Quat b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}
Quat qconj(Quat q) { return {-q.x, -q.y, -q.z, q.w}; }
Vec3 qrot(Quat q, Vec3 v) {
    const Vec3 u{q.x, q.y, q.z};
    const Vec3 t{2 * (u.y * v.z - u.z * v.y), 2 * (u.z * v.x - u.x * v.z), 2 * (u.x * v.y - u.y * v.x)};
    return {v.x + q.w * t.x + (u.y * t.z - u.z * t.y), v.y + q.w * t.y + (u.z * t.x - u.x * t.z), v.z + q.w * t.z + (u.x * t.y - u.y * t.x)};
}

std::string g_active;
std::map<std::string, API::FName> g_names;   // finger bone name -> FName, made once

API::FName fname(const std::string& s) {
    auto it = g_names.find(s);
    if (it != g_names.end()) return it->second;
    const API::FName n = ue::name(std::wstring(s.begin(), s.end()));
    g_names[s] = n;
    return n;
}

const json::Value* entry(const std::string& key) {
    const json::Value* f = params::file("sh2vr_front_grips.json");
    return f != nullptr ? f->get(key) : nullptr;
}
bool nums(const json::Value* v, size_t n, double* out) {
    if (v == nullptr || v->kind != json::Value::ARR || v->arr.size() < n) return false;
    for (size_t i = 0; i < n; ++i) out[i] = v->arr[i].n;
    return true;
}

// ---- the capture ----
std::string fmt(double v) { char b[32]; std::snprintf(b, sizeof b, "%.6f", v); return b; }

struct Captured {
    Vec3 pos; Quat rot;
    std::vector<std::pair<std::string, Rot>> fingers;
};

// the saved file, our own text: every gun already in it, with this one added or replaced
bool write_file(const std::string& key, const Captured& c) {
    std::map<std::string, std::string> blocks;
    if (const json::Value* f = params::file("sh2vr_front_grips.json"); f != nullptr && f->kind == json::Value::OBJ) {
        for (const auto& [k, v] : f->obj) {
            if (k == key || k.rfind("_", 0) == 0) continue;
            double p[3], q[4];
            if (!nums(v.get("pos"), 3, p) || !nums(v.get("rot"), 4, q)) continue;
            std::string b = "  \"" + k + "\": {\"pos\": [" + fmt(p[0]) + ", " + fmt(p[1]) + ", " + fmt(p[2]) + "], \"rot\": [" + fmt(q[0]) + ", " + fmt(q[1]) + ", " + fmt(q[2]) + ", " + fmt(q[3]) + "], \"fingers\": {";
            bool first = true;
            if (const json::Value* fg = v.get("fingers"); fg != nullptr && fg->kind == json::Value::OBJ) {
                for (const auto& [bone, r] : fg->obj) {
                    double a[3];
                    if (!nums(&r, 3, a)) continue;
                    b += std::string(first ? "" : ", ") + "\"" + bone + "\": [" + fmt(a[0]) + ", " + fmt(a[1]) + ", " + fmt(a[2]) + "]";
                    first = false;
                }
            }
            blocks[k] = b + "}}";
        }
    }
    std::string b = "  \"" + key + "\": {\"pos\": [" + fmt(c.pos.x) + ", " + fmt(c.pos.y) + ", " + fmt(c.pos.z) + "], \"rot\": [" + fmt(c.rot.x) + ", " + fmt(c.rot.y) + ", " + fmt(c.rot.z) + ", " + fmt(c.rot.w) + "], \"fingers\": {";
    for (size_t i = 0; i < c.fingers.size(); ++i) {
        const Rot& r = c.fingers[i].second;
        b += std::string(i ? ", " : "") + "\"" + c.fingers[i].first + "\": [" + fmt(r.pitch) + ", " + fmt(r.yaw) + ", " + fmt(r.roll) + "]";
    }
    blocks[key] = b + "}}";

    std::string text = "{\n  \"_about\": \"sh2vr front grips: the game's own left hand on each long gun's front (its aim pose), against the weapon; pos cm, rot quaternion x y z w, fingers pitch yaw roll in parent-bone space. Written by frontgrip.cpp (F2 while the game aims).\"";
    for (const auto& [k, blk] : blocks) text += ",\n" + blk;
    text += "\n}\n";
    const std::wstring path = appdata_dir() + L"\\" + settings::PROFILE_DIR + L"\\" + settings::PARAMS_SUBDIR + L"\\sh2vr_front_grips.json";
    FILE* fp = nullptr;
    if (_wfopen_s(&fp, path.c_str(), L"wb") != 0 || fp == nullptr) return false;
    const bool ok = std::fwrite(text.data(), 1, text.size(), fp) == text.size();
    std::fclose(fp);
    return ok;
}

void capture() {
    API::UObject* w = gun::equipped();
    const std::string key = key_of(w);
    if (key.empty()) { LOGW("%s frontgrip: F2: no shotgun or rifle in hand (%s)", TAG, ue::name_of(w).c_str()); return; }
    auto** body = ue::prop<API::UObject*>(gun::pawn(), L"Mesh");
    auto** root = ue::prop<API::UObject*>(w, L"RootComponent");
    Transform r, h;
    const API::FName hand = ue::name(settings::FRONT_GRIP_HAND_BONE);
    if (body == nullptr || !ue::alive(*body) || root == nullptr || !ue::alive(*root) ||
        !ue::call_ret(*root, L"K2_GetComponentToWorld", {}, r) || !ue::call_ret(*body, L"GetSocketTransform", {hand, (int32_t)RTS_WORLD}, h)) {
        LOGW("%s frontgrip: F2: could not read the body's hand or the %s's frame", TAG, key.c_str());
        return;
    }
    Captured c;
    c.pos = qrot(qconj(r.rot), Vec3{h.pos.x - r.pos.x, h.pos.y - r.pos.y, h.pos.z - r.pos.z});
    c.rot = qmul(qconj(r.rot), h.rot);
    // every bone below the hand: its turn against its parent, as the game's animation has it now
    int32_t n = 0;
    ue::call_ret(*body, L"GetNumBones", {}, n);
    const std::wstring hand_w = settings::FRONT_GRIP_HAND_BONE;
    for (int32_t i = 0; i < n; ++i) {
        API::FName b;
        if (!ue::call_ret(*body, L"GetBoneName", {i}, b)) continue;
        API::FName p = b;
        bool under = false;
        for (int up = 0; up < settings::FRONT_GRIP_FINGER_DEPTH; ++up) {
            API::FName q;
            if (!ue::call_ret(*body, L"GetParentBone", {p}, q) || ue::is_none(q)) break;
            if (ue::str(q) == hand_w) { under = true; break; }
            p = q;
        }
        if (!under) continue;
        Transform local;
        if (!ue::call_ret(*body, L"GetSocketTransform", {b, (int32_t)RTS_PARENT_BONE}, local)) continue;
        c.fingers.push_back({narrow(ue::str(b)), gun::rot_of(local)});
    }
    const double dist = std::sqrt(c.pos.x * c.pos.x + c.pos.y * c.pos.y + c.pos.z * c.pos.z);
    // only the game's real grip: the body's hand on the gun's own left-hand socket (else it is some other animation)
    Transform hs;
    if (gun::socket_xform(gun::mesh(w), settings::FRONT_GRIP_SOCKET, hs)) {
        auto* m = gun::mesh(w);
        Transform mw;
        if (ue::call_ret(m, L"K2_GetComponentToWorld", {}, mw)) {
            const Vec3 sl = qrot(qconj(mw.rot), Vec3{hs.pos.x - mw.pos.x, hs.pos.y - mw.pos.y, hs.pos.z - mw.pos.z});
            const double off = std::sqrt((sl.x - c.pos.x) * (sl.x - c.pos.x) + (sl.y - c.pos.y) * (sl.y - c.pos.y) + (sl.z - c.pos.z) * (sl.z - c.pos.z));
            if (off > settings::FRONT_GRIP_CAPTURE_MATCH_CM) {
                LOGW("%s frontgrip: F2: the body's left hand is %.1f cm off the %s's %ls: not the game's grip right now, nothing saved", TAG, off, key.c_str(), settings::FRONT_GRIP_SOCKET);
                return;
            }
        }
    }
    const bool ok = write_file(key, c);
    LOGI("%s frontgrip: F2: the game's left hand on the %s: %.1f %.1f %.1f cm (%.1f from its frame), %zu finger bones -> %s", TAG,
         key.c_str(), c.pos.x, c.pos.y, c.pos.z, dist, c.fingers.size(), ok ? "saved" : "NOT saved (file)");
    gun::buzz(controllers::LEFT, settings::BUZZ_GRAB_AMP, settings::BUZZ_S);
}
} // namespace

void on_message(unsigned msg, unsigned long long wparam) {
    if (msg != WM_KEYDOWN && msg != WM_SYSKEYDOWN) return;
    if (wparam == settings::FRONT_GRIP_CAPTURE_KEY) capture();
}

std::string key_of(API::UObject* weapon) {
    if (!ue::alive(weapon)) return "";
    const std::string n = ue::name_of((API::UObject*)weapon->get_class());
    if (n.find("Shotgun") != std::string::npos) return "Shotgun";
    if (n.find("Rifle") != std::string::npos) return "Rifle";
    return "";
}

bool target(const std::string& key, API::UObject* gun_mesh, const Transform& gun_world, double slide_cm, Vec3& pos, Quat& rot) {
    if (!settings::FRONT_GRIP_ON || key.empty()) return false;
    const Vec3 back = qrot(gun_world.rot, Vec3{settings::TWO_HAND_BARREL_AXIS[0] * slide_cm, settings::TWO_HAND_BARREL_AXIS[1] * slide_cm,
                                               settings::TWO_HAND_BARREL_AXIS[2] * slide_cm});
    Transform s;
    if (gun::socket_xform(gun_mesh, settings::FRONT_GRIP_SOCKET, s)) {   // the game's own grip, live
        pos = {s.pos.x - back.x, s.pos.y - back.y, s.pos.z - back.z};
        rot = s.rot;
        return true;
    }
    const json::Value* e = entry(key);
    double p[3], q[4];
    if (e == nullptr || !nums(e->get("pos"), 3, p) || !nums(e->get("rot"), 4, q)) return false;
    // back along the barrel by the pump's slide (the barrel is the gun mesh's +Y, settings::TWO_HAND_BARREL_AXIS)
    const Vec3 local{p[0] - settings::TWO_HAND_BARREL_AXIS[0] * slide_cm, p[1] - settings::TWO_HAND_BARREL_AXIS[1] * slide_cm,
                     p[2] - settings::TWO_HAND_BARREL_AXIS[2] * slide_cm};
    const Vec3 off = qrot(gun_world.rot, local);
    pos = {gun_world.pos.x + off.x, gun_world.pos.y + off.y, gun_world.pos.z + off.z};
    rot = qmul(gun_world.rot, Quat{q[0], q[1], q[2], q[3]});
    return true;
}

void set_active(const std::string& key) {
    if (key == g_active) return;
    g_active = key;
    LOGI("%s frontgrip: %s", TAG, key.empty() ? "the left fingers back to the pack's poses" : ("the left fingers in the game's grip on the " + key).c_str());
}

void tick() {
    if (g_active.empty() || !settings::FRONT_GRIP_ON) return;
    API::UObject* m = ik::mesh();
    const json::Value* e = entry(g_active);
    const json::Value* fg = e != nullptr ? e->get("fingers") : nullptr;
    if (!ue::alive(m)) return;
    if (fg == nullptr || fg->kind != json::Value::OBJ || fg->obj.empty()) {   // no game fingers saved yet: the pack's own pose for it
        fingers::hold_pose(fingers::LEFT, g_active == "Shotgun" ? settings::FRONT_GRIP_POSE_SHOTGUN : settings::FRONT_GRIP_POSE_RIFLE);
        return;
    }
    for (const auto& [bone, r] : fg->obj) {
        double a[3];
        if (nums(&r, 3, a)) anim::set_local_rotator(m, fname(bone), Rot{a[0], a[1], a[2]});
    }
}

} // namespace sh2vr::frontgrip
