// reload.cpp -- see reload.h. The states are Visceral RE8's reload.cpp's, cut down to the pistol magazine; the
// magazine's drawing is the game's: the pistol's own magazine bones are hidden while it is out, the game's
// HandgunAmmoClosed static mesh rides the left controller while one is carried (the pack's numbers for its fit).
#include "reload.h"

#include <cmath>
#include <initializer_list>
#include <string>

#include "ammo.h"
#include "ghosthand.h"
#include "gun.h"
#include "holsters.h"
#include "ikmath.h"
#include "input.h"
#include "rack.h"
#include "settings.h"

namespace sh2vr::reload {

using namespace sh2vr::ikmath;
using gun::Rot;
using gun::Transform;
using gun::Vec3;

namespace {
enum class St { SEATED, DROPPED, IN_HAND };   // not OUT: a Windows macro
const char* st_name(St s) { return s == St::SEATED ? "seated" : s == St::DROPPED ? "out" : "in the left hand"; }

St g_st = St::SEATED;
API::UObject* g_weapon = nullptr;
API::UObject* g_hand_mesh = nullptr;   // the magazine's carrier on the left controller (the fit); visible only as a fallback
API::UObject* g_hand_copy = nullptr;   // the pistol's own model, only its magazine showing, driven to the carrier each tick
int g_mag_rounds = 0;                  // rounds that left with the magazine
bool g_dry = false;                    // the gun was empty when its magazine came out
bool g_in_pouch = false;
bool g_bones_hidden = false;
float g_since_grab = 99.0f;
double g_closest = 9999.0;
int g_mesh_missing_logs = 0;

void set_state(St s, const char* why) {
    LOGI("%s reload: magazine %s -> %s (%s)", TAG, st_name(g_st), st_name(s), why);
    g_st = s;
}

void mag_bones(API::UObject* w, bool hidden) {
    if (hidden == g_bones_hidden) return;
    API::UObject* m = gun::mesh(w);
    if (m == nullptr) return;
    for (const wchar_t* b : settings::MAG_BONES) {
        if (hidden) ue::call(m, L"HideBoneByName", {ue::name(b), (int32_t)0});   // EPhysBodyOp::PBO_None
        else ue::call(m, L"UnHideBoneByName", {ue::name(b)});
    }
    g_bones_hidden = hidden;
}

// the ammo spot: holsters::left_hand_to_spot_cm with the pack's holster numbers (settings HIP_*)

void destroy_hand_mesh() {
    if (ue::alive(g_hand_mesh)) ue::call(g_hand_mesh, L"K2_DestroyComponent", {g_hand_mesh});
    if (ue::alive(g_hand_copy)) ue::call(g_hand_copy, L"K2_DestroyComponent", {g_hand_copy});
    g_hand_mesh = nullptr;
    g_hand_copy = nullptr;
}

bool listed(const std::wstring& name, std::initializer_list<const wchar_t*> list) {
    for (const wchar_t* n : list) if (name == n) return true;
    return false;
}

// the pistol's own model as a poseable copy with only the magazine left (LukasBlaster7's showOnlyPistolMagazine):
// every bone but the magazine's shrunk to nothing in world space (parents first, so the magazine's own scale of one
// holds), every bone but the magazine and its two ancestors hidden. nullptr when it cannot be made.
API::UObject* make_magazine_copy(API::UObject* owner, API::UObject* pistol_mesh) {
    auto* cls = ue::find_class(L"Class /Script/Engine.PoseableMeshComponent");
    if (cls == nullptr || pistol_mesh == nullptr) return nullptr;
    Transform t;
    API::UObject* comp = nullptr;
    if (!ue::call_ret(owner, L"AddComponentByClass", {(API::UObject*)cls, true, t, false}, comp) || comp == nullptr) return nullptr;
    auto** dst = ue::prop<API::UObject*>(comp, L"SkeletalMesh");
    auto** src = ue::prop<API::UObject*>(pistol_mesh, L"SkeletalMesh");
    if (dst == nullptr || src == nullptr || *src == nullptr) { ue::call(comp, L"K2_DestroyComponent", {comp}); return nullptr; }
    *dst = *src;
    const wchar_t* follow = L"SetLeaderPoseComponent";   // the first update, then on its own (uevr_utils does the same)
    if (!ue::call(comp, follow, {pistol_mesh, true})) { follow = L"SetMasterPoseComponent"; ue::call(comp, follow, {pistol_mesh, true}); }
    ue::call(comp, follow, {(API::UObject*)nullptr, false});
    ue::call(comp, L"CopyPoseFromSkeletalComponent", {pistol_mesh});
    int32_t mats = 0;
    ue::call_ret(pistol_mesh, L"GetNumMaterials", {}, mats);
    for (int32_t i = 0; i < mats; ++i) { API::UObject* m = nullptr; if (ue::call_ret(pistol_mesh, L"GetMaterial", {i}, m) && m != nullptr) ue::call(comp, L"SetMaterial", {i, m}); }
    int32_t bones = 0;
    ue::call_ret(comp, L"GetNumBones", {}, bones);
    int kept = 0;
    for (int32_t i = 0; i < bones; ++i) {
        API::FName fn{};
        if (!ue::call_ret(comp, L"GetBoneName", {i}, fn)) continue;
        const std::wstring n = ue::str(fn);
        const bool keep = listed(n, {settings::HAND_MAG_KEEP[0], settings::HAND_MAG_KEEP[1]});
        const double sc = keep ? 1.0 : 0.001;
        ue::call(comp, L"SetBoneScaleByName", {fn, Vec3{sc, sc, sc}, (int32_t)0});   // EBoneSpaces::WorldSpace
        if (keep) { ue::call(comp, L"UnHideBoneByName", {fn}); ++kept; }
        else if (!listed(n, {settings::HAND_MAG_ANCESTORS[0], settings::HAND_MAG_ANCESTORS[1]})) ue::call(comp, L"HideBoneByName", {fn, (int32_t)0});
    }
    if (auto* b = ue::prop<float>(comp, L"BoundsScale")) *b = settings::HAND_MAG_BOUNDS_SCALE;
    ue::call(comp, L"SetCollisionEnabled", {(int32_t)0});
    ue::call(comp, L"SetVisibility", {true});
    ue::call(comp, L"SetHiddenInGame", {false});
    static int said = 0;
    if (said++ < 3) LOGI("%s reload: the hand magazine is the pistol's own model, %d of %d bones kept", TAG, kept, bones);
    if (kept == 0) { ue::call(comp, L"K2_DestroyComponent", {comp}); return nullptr; }
    return comp;
}

// each tick while it is in the hand: the copy's magazine bone onto the carrier (the hand's fit)
void drive_magazine_copy() {
    if (!ue::alive(g_hand_copy) || !ue::alive(g_hand_mesh)) return;
    Vec3 loc;
    Rot rot;
    if (!ue::call_ret(g_hand_mesh, L"K2_GetComponentLocation", {}, loc) || !ue::call_ret(g_hand_mesh, L"K2_GetComponentRotation", {}, rot)) return;
    const API::FName bone = ue::name(settings::HAND_MAG_BONE);
    ue::call(g_hand_copy, L"SetBoneLocationByName", {bone, loc, (int32_t)0});
    ue::call(g_hand_copy, L"SetBoneRotationByName", {bone, rot, (int32_t)0});
    ue::call(g_hand_copy, L"MarkRenderTransformDirty", {});
}

// the game's magazine mesh on the left controller (uevr_utils.createStaticMeshComponent + the pack's hand-mag fit)
void make_hand_mesh(API::UObject* pistol) {
    destroy_hand_mesh();
    API::UObject* ctrl = controllers::find(controllers::LEFT);
    auto* asset = API::get()->find_uobject<API::UObject>(settings::HAND_MAG_MESH);
    auto* cls = ue::find_class(L"Class /Script/Engine.StaticMeshComponent");
    API::UObject* owner = nullptr;
    if (ctrl != nullptr) ue::call_ret(ctrl, L"GetOwner", {}, owner);
    if (asset == nullptr || cls == nullptr || owner == nullptr) {
        if (g_mesh_missing_logs++ < 3) LOGW("%s reload: no magazine to draw in the hand (mesh asset %s, component class %s, controller owner %s); the gesture still works", TAG, asset ? "found" : "NOT LOADED", cls ? "ok" : "missing", owner ? "ok" : "missing");
        return;
    }
    Transform t;
    API::UObject* comp = nullptr;
    if (!ue::call_ret(owner, L"AddComponentByClass", {(API::UObject*)cls, true, t, false}, comp) || comp == nullptr) { LOGW("%s reload: could not make the hand magazine component", TAG); return; }
    ue::call(comp, L"SetStaticMesh", {asset});
    ue::call(comp, L"SetCollisionEnabled", {(int32_t)0});
    bool attached = false;
    ue::call_ret(comp, L"K2_AttachTo", {ctrl, ue::name(L""), (int32_t)0, false}, attached);
    ue::call(comp, L"K2_SetRelativeLocationAndRotation", {Vec3{settings::HAND_MAG_LOC[0], settings::HAND_MAG_LOC[1], settings::HAND_MAG_LOC[2]},
                                                           Rot{settings::HAND_MAG_ROT[0], settings::HAND_MAG_ROT[1], settings::HAND_MAG_ROT[2]}, false, false});
    g_hand_mesh = comp;
    g_hand_copy = settings::HAND_MAG_FROM_PISTOL ? make_magazine_copy(owner, gun::mesh(pistol)) : nullptr;
    const bool box = g_hand_copy == nullptr;   // the copy failed: the ammo box shows, as before
    ue::call(comp, L"SetVisibility", {box});
    ue::call(comp, L"SetHiddenInGame", {!box});
    if (box && settings::HAND_MAG_FROM_PISTOL) LOGW("%s reload: could not copy the pistol's magazine; the ammo box shows instead", TAG);
    drive_magazine_copy();
}

void drop(API::UObject* w) {
    g_mag_rounds = ammo::rounds(w);
    g_dry = g_mag_rounds <= 0;
    ammo::set_rounds(w, 0);
    mag_bones(w, true);
    set_state(St::DROPPED, "Quest B");
    gun::buzz(controllers::RIGHT, settings::BUZZ_GRAB_AMP, settings::BUZZ_S);
    LOGI("%s reload: magazine out with %d round(s) in it, %d carried", TAG, g_mag_rounds, ammo::carried(w));
}

void grab(API::UObject* w) {
    if (ammo::carried(w) <= 0 && g_mag_rounds <= 0) {
        LOGI("%s reload: the ammo spot is empty (nothing carried, none in the dropped magazine)", TAG);
        gun::buzz(controllers::LEFT, settings::BUZZ_DENY_AMP, settings::BUZZ_S);
        return;
    }
    make_hand_mesh(w);
    g_closest = 9999.0;
    set_state(St::IN_HAND, "taken from the ammo spot");
    ghosthand::event("mag", "start");
    gun::buzz(controllers::LEFT, settings::BUZZ_GRAB_AMP, settings::BUZZ_S);
}

void insert(API::UObject* w, double d) {
    const int cap = ammo::clip_size(w);
    const int want = cap - g_mag_rounds;
    const int took = want > 0 ? ammo::take_carried(w, want) : 0;
    const int total = g_mag_rounds + took;
    ammo::fill(w, total);
    g_mag_rounds = 0;
    destroy_hand_mesh();
    mag_bones(w, false);
    set_state(St::SEATED, "brought to the gun");
    ghosthand::value("mag_closest_cm", (float)d);
    ghosthand::event("mag", "done");
    gun::buzz(controllers::LEFT, settings::BUZZ_DONE_AMP, settings::BUZZ_S);
    if (g_dry && total > 0) rack::need("a magazine into an empty gun");
    g_dry = false;
    LOGI("%s reload: new magazine: %d round(s) (%d from the carried ammo, capacity %d), %d carried now", TAG, total, took, cap, ammo::carried(w));
}
} // namespace

bool managed() { return g_weapon != nullptr && gun::hmd_active(); }
bool mag_out() { return managed() && g_st != St::SEATED; }
bool mag_in_hand() { return managed() && g_st == St::IN_HAND; }
bool hand_busy() { return mag_in_hand(); }
bool blocks_fire() { return mag_out(); }
void on_racked() {}

void reset(const char* why) {
    if (g_st != St::SEATED) LOGI("%s reload: magazine put back as it was (%s): %d round(s)", TAG, why, g_mag_rounds);
    if (g_st != St::SEATED && ue::alive(g_weapon)) ammo::set_rounds(g_weapon, g_mag_rounds);
    destroy_hand_mesh();
    if (ue::alive(g_weapon)) mag_bones(g_weapon, false);
    g_bones_hidden = false;
    g_st = St::SEATED;
    g_mag_rounds = 0;
    g_dry = false;
    g_weapon = nullptr;
}

void tick(float delta) {
    g_since_grab += delta;
    API::UObject* w = gun::equipped();
    if (!gun::is_pistol(w) || !gun::hmd_active()) { if (g_weapon != nullptr) reset(gun::is_pistol(w) ? "no headset" : "no pistol in hand"); return; }
    if (w != g_weapon) { if (g_weapon != nullptr) reset("another pistol"); g_weapon = w; }

    Vec3 left;
    Rot left_rot;
    if (!gun::controller(controllers::LEFT, left, left_rot)) return;
    // 2026-10-31: measured like the holsters (headset tracking space, yaw only); the game's camera did not follow the
    // headset and Tefa found no pouch at all ("i pressed LG around left hip all over and not a single hit")
    double d = 0;
    if (holsters::left_hand_to_spot_cm(settings::HIP_RIGHT_CM, settings::HIP_UP_CM, settings::HIP_FORWARD_CM, d)) {
        if (!g_in_pouch && d < settings::HIP_ENTER_CM) { g_in_pouch = true; if (g_st == St::DROPPED) gun::buzz(controllers::LEFT, settings::BUZZ_DENY_AMP, settings::BUZZ_S); }
        else if (g_in_pouch && d > settings::HIP_LEAVE_CM) g_in_pouch = false;
    }
    const bool lg = input::grip(false);

    switch (g_st) {
    case St::SEATED:
        if (input::pressed(settings::QUEST_B) && !rack::active()) drop(w);   // Quest B (the pad's X, settings.h)
        break;
    case St::DROPPED:
        if (input::grip_pressed(false) && g_in_pouch && g_since_grab >= settings::RELOAD_GRAB_COOLDOWN_S) { g_since_grab = 0.0f; grab(w); }
        break;
    case St::IN_HAND: {
        if (!lg) {
            LOGI("%s reload: let go; it came within %.1f cm of the seat (needs %.0f)", TAG, g_closest, settings::MAG_DOCK_CM);
            ghosthand::value("mag_closest_cm", (float)g_closest);
            ghosthand::event("mag", "fail");
            destroy_hand_mesh();
            set_state(St::DROPPED, "left grip let go: it drops");
            break;
        }
        drive_magazine_copy();
        Vec3 seat;
        if (!gun::socket_loc(gun::mesh(w), settings::MAG_SOCKET, seat)) break;
        const double d = size(sub(left, seat));
        g_closest = std::fmin(g_closest, d);
        if (d <= settings::MAG_DOCK_CM) insert(w, d);
        break;
    }
    }
    ghosthand::value("loaded", (float)ammo::rounds(w));
    ghosthand::value("mag_in_hand", g_st == St::IN_HAND ? 1.0f : 0.0f);
}

} // namespace sh2vr::reload
