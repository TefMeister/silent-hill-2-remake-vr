// shadows.cpp -- see shadows.h. [compile-verified 2026-11-06] as a plugin; every step was done by hand through the
// connector on the live game the same evening and read back, so the calls and their effects are known; the plugin
// doing them by itself is what has not run yet.
#include "shadows.h"

#include "cutscreen.h"

#include <cstdio>
#include <set>
#include <string>
#include <vector>

#include "common.h"
#include "gun.h"
#include "ik.h"
#include "settings.h"
#include "ue.h"

namespace sh2vr::shadows {

namespace {

struct Channels { bool c0 = false, c1 = false, c2 = false; };

API::UObject* g_pawn = nullptr;
API::UObject* g_body = nullptr;
API::UObject* g_copy = nullptr;             // the shadow-only body
API::UObject* g_weapon_mesh_done = nullptr; // the last weapon mesh given channel 0
std::vector<int32_t> g_head_slots;          // the body's head material slots, found by their material names once
bool g_head_slots_known = false;
std::vector<API::UObject*> g_head_mats;     // each head slot's own material, put back while a cutscene plays on the flat screen (2026-11-12)
bool g_head_shown = false;
float g_since = settings::SHADOWS_EVERY_S;  // so the first tick works at once
int g_body_fixes = 0;

bool channels_of(API::UObject* c, Channels& out) {
    auto* p = ue::prop<uint8_t>(c, L"LightingChannels");   // FLightingChannels: three bitfield bools in one byte
    if (p == nullptr) return false;
    out.c0 = (*p & 1) != 0; out.c1 = (*p & 2) != 0; out.c2 = (*p & 4) != 0;
    return true;
}

// a bitfield bool, set through the property's own mask (a plain byte write would take the neighbouring flags with it)
bool set_bool(API::UObject* o, const wchar_t* name, bool v) {
    if (!ue::alive(o)) return false;
    auto* p = o->get_class()->find_property(name);
    if (p == nullptr) return false;
    static_cast<API::FBoolProperty*>(p)->set_value_in_object(o, v);
    return static_cast<API::FBoolProperty*>(p)->get_value_from_object(o) == v;
}
bool get_bool(API::UObject* o, const wchar_t* name, bool& v) {
    if (!ue::alive(o)) return false;
    auto* p = o->get_class()->find_property(name);
    if (p == nullptr) return false;
    v = static_cast<API::FBoolProperty*>(p)->get_value_from_object(o);
    return true;
}

API::UObject* invisible_material() {
    static API::UObject* m = nullptr;
    if (!ue::alive(m)) m = API::get()->find_uobject<API::UObject>(settings::SHADOWS_INVISIBLE_MATERIAL);
    return m;
}

bool name_says_head(const std::string& material_name) {
    for (const char* k : settings::SHADOWS_HEAD_MATERIAL_WORDS) if (material_name.find(k) != std::string::npos) return true;
    return false;
}

// 1. the weapon in hand: lit and shadowed by the torch (channel 0 added, channel 1 kept for the body's fill lights)
void weapon_channel() {
    API::UObject* m = gun::mesh(gun::equipped());
    if (m == nullptr || m == g_weapon_mesh_done) return;
    Channels ch;
    if (!channels_of(m, ch)) return;
    if (!ch.c0) {
        ue::call(m, L"SetLightingChannels", {true, true, ch.c2});   // 0 for the torch, 1 kept for the body's fill lights
        Channels after;
        channels_of(m, after);
        LOGI("%s shadows: the %s's mesh put on the torch's lighting channel (channels %d%d%d -> %d%d%d)", TAG, ue::name_of((API::UObject*)gun::equipped()->get_class()).c_str(),
             (int)ch.c0, (int)ch.c1, (int)ch.c2, (int)after.c0, (int)after.c1, (int)after.c2);
    }
    g_weapon_mesh_done = m;
}

// 2. the body: the neck bone shown, the head's slots invisible, its own shadow off
void body_fix() {
    if (!ue::alive(g_body)) return;
    API::UObject* inv = invisible_material();
    if (inv == nullptr) { static bool said = false; if (!said) { LOGW("%s shadows: the game's invisible material was not found: the head stays hidden by its bone, no head in the shadow", TAG); said = true; } return; }
    if (!g_head_slots_known) {
        int32_t n = 0;
        if (!ue::call_ret(g_body, L"GetNumMaterials", {}, n) || n <= 0) return;
        for (int32_t i = 0; i < n; ++i) {
            API::UObject* mat = nullptr;
            if (ue::call_ret(g_body, L"GetMaterial", {i}, mat) && mat != nullptr && name_says_head(ue::name_of(mat))) { g_head_slots.push_back(i); g_head_mats.push_back(mat); }
        }
        g_head_slots_known = !g_head_slots.empty();
        if (g_head_slots_known) {
            std::string s;
            for (int32_t i : g_head_slots) s += (s.empty() ? "" : " ") + std::to_string(i);
            LOGI("%s shadows: the body's head is %d of %d material slots (%s)", TAG, (int)g_head_slots.size(), (int)n, s.c_str());
        }
        return;
    }
    int changed = 0;
    // 2026-11-12: a cutscene on the flat screen shows the character from outside: the head's own materials are back
    // for as long as it plays (cutscreen.cpp), invisible again after.
    g_head_shown = cutscreen::active();
    for (size_t k = 0; k < g_head_slots.size(); ++k) {
        const int32_t i = g_head_slots[k];
        API::UObject* want = g_head_shown && ue::alive(g_head_mats[k]) ? g_head_mats[k] : inv;
        API::UObject* mat = nullptr;
        if (ue::call_ret(g_body, L"GetMaterial", {i}, mat) && mat != want) { ue::call(g_body, L"SetMaterial", {i, want}); ++changed; }
    }
    bool hidden = false;
    const API::FName neck = ue::name(settings::SHADOWS_HEAD_BONE);
    if (ue::call_ret(g_body, L"IsBoneHiddenByName", {neck}, hidden) && hidden) { ue::call(g_body, L"UnHideBoneByName", {neck}); ++changed; }
    bool cast = true;
    if (get_bool(g_body, L"CastShadow", cast) && cast) { ue::call(g_body, L"SetCastShadow", {false}); ++changed; }
    if (changed > 0) {
        ++g_body_fixes;
        if (g_body_fixes <= 3 || g_body_fixes % 50 == 0)
            LOGI("%s shadows: body fix %d: %d thing(s) put right (head slots invisible, neck bone shown, its own shadow off)", TAG, g_body_fixes, changed);
    }
}

// 3. the shadow-only body
void make_copy() {
    if (ue::alive(g_copy) || !ue::alive(g_body) || !ue::alive(g_pawn)) return;
    auto* cls = ue::find_class(L"Class /Script/Engine.SkeletalMeshComponent");
    API::UObject* asset = nullptr;
    if (cls == nullptr) return;
    if (auto** a = ue::prop<API::UObject*>(g_body, L"SkeletalMesh")) asset = *a;
    if (!ue::alive(asset)) return;
    API::UObject* c = nullptr;
    ue::Transform identity;
    if (!ue::call_ret(g_pawn, L"AddComponentByClass", {(API::UObject*)cls, true, identity, false}, c) || c == nullptr) { LOGW("%s shadows: could not add the shadow body", TAG); g_copy = g_pawn; return; }   // g_pawn as a "tried" mark: no retry storm
    ue::call(c, L"SetSkeletalMeshAsset", {asset});
    bool attached = false;
    ue::call_ret(c, L"K2_AttachTo", {g_body, ue::name(L""), (int32_t)0, false}, attached);
    ue::call(c, L"SetLeaderPoseComponent", {g_body, true, false});
    ue::call(c, L"SetLightingChannels", {true, true, false});
    ue::call(c, L"SetHiddenInGame", {true, false});
    ue::call(c, L"SetCastShadow", {true});
    const bool hidden_shadow = set_bool(c, L"bCastHiddenShadow", true);
    g_copy = c;
    Channels ch; channels_of(c, ch);
    bool hid = false; get_bool(c, L"bHiddenInGame", hid);
    LOGI("%s shadows: the shadow body made on the pawn: follows the body %s, attached %s, hidden %d, casts hidden shadow %d, channels %d%d%d (%s)", TAG,
         ue::name_of(g_body).c_str(), attached ? "yes" : "NO", (int)hid, (int)hidden_shadow, (int)ch.c0, (int)ch.c1, (int)ch.c2, ue::name_of(c).c_str());
}

} // namespace

void tick(float delta) {
    if (!settings::SHADOWS_ON) return;
    g_since += delta;
    if (g_since < settings::SHADOWS_EVERY_S && cutscreen::active() == g_head_shown) return;   // a cutscene beginning or ending is acted on at once
    g_since = 0.0f;
    API::UObject* pawn = API::get()->get_local_pawn(0);
    if (pawn != g_pawn) {   // a new pawn (a level, a death): start over; the old copy died with its pawn
        g_pawn = pawn; g_body = nullptr; g_copy = nullptr; g_weapon_mesh_done = nullptr; g_head_slots.clear(); g_head_mats.clear(); g_head_slots_known = false; g_body_fixes = 0;
    }
    if (pawn == nullptr) return;
    if (!ue::alive(g_body)) { if (auto** b = ue::prop<API::UObject*>(pawn, L"Mesh")) g_body = ue::alive(*b) ? *b : nullptr; }
    if (!ue::alive(g_body)) return;
    if (settings::SHADOWS_WEAPON_ON_TORCH) weapon_channel();
    if (settings::SHADOWS_HEAD_IN_SHADOW) { body_fix(); if (g_head_slots_known) make_copy(); }
}

} // namespace sh2vr::shadows
