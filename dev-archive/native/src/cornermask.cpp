// cornermask.cpp -- see cornermask.h.
#include "cornermask.h"

#include <windows.h>

#include <cwchar>
#include <fstream>
#include <string>
#include <vector>

#include "common.h"
#include "settings.h"
#include "ue.h"
#include "uevr/API.hpp"

namespace sh2vr::cornermask {

namespace {
constexpr float CHECK_S = 1.0f;
constexpr const wchar_t* FILE_NAME = L"sh2vr_mask.txt";
constexpr uint32_t HAND_HMD = 2;   // UEVR's motion controller state: 0 left, 1 right, 2 the headset
// the engine's own 100 cm shapes, tried in this order (whichever this build has loaded)
constexpr const wchar_t* SHAPES[] = {
    L"StaticMesh /Engine/BasicShapes/Cube.Cube",
    L"StaticMesh /Engine/EngineMeshes/Cube.Cube",
    L"StaticMesh /Engine/BasicShapes/Plane.Plane",
};

float g_since = CHECK_S;
FILETIME g_seen{};
API::UObject* g_anchor = nullptr;            // follows the headset (UEVR moves it at view time)
std::vector<API::UObject*> g_pieces;

std::wstring path() { return appdata_dir() + L"\\" + settings::PROFILE_DIR + L"\\" + FILE_NAME; }

// 1 = the file changed, -1 = the file is gone (and was there), 0 = nothing new
int changed(const std::wstring& p) {
    WIN32_FILE_ATTRIBUTE_DATA a{};
    if (!GetFileAttributesExW(p.c_str(), GetFileExInfoStandard, &a)) {
        const bool was = g_seen.dwLowDateTime != 0 || g_seen.dwHighDateTime != 0;
        g_seen = FILETIME{};
        return was ? -1 : 0;
    }
    if (CompareFileTime(&a.ftLastWriteTime, &g_seen) == 0) return 0;
    g_seen = a.ftLastWriteTime;
    return 1;
}

void clear() {
    for (API::UObject* c : g_pieces) if (ue::alive(c)) ue::call(c, L"K2_DestroyComponent", {c});
    if (!g_pieces.empty()) LOGI("%s cornermask: %zu piece(s) removed", TAG, g_pieces.size());
    g_pieces.clear();
}

API::UObject* shape() {
    for (const wchar_t* s : SHAPES)
        if (auto* m = API::get()->find_uobject<API::UObject>(s)) return m;
    return nullptr;
}

bool anchor(API::UObject* pawn) {
    if (ue::alive(g_anchor)) return true;
    auto* cls = ue::find_class(L"Class /Script/Engine.SceneComponent");
    ue::Transform t;
    API::UObject* comp = nullptr;
    if (cls == nullptr || !ue::call_ret(pawn, L"AddComponentByClass", {(API::UObject*)cls, true, t, false}, comp) || comp == nullptr) return false;
    auto* st = API::UObjectHook::get_or_add_motion_controller_state(comp);
    if (st == nullptr) return false;
    st->set_hand(HAND_HMD);
    st->set_permanent(true);
    g_anchor = comp;
    return true;
}

void build(const std::wstring& p) {
    clear();
    API::UObject* pawn = API::get()->get_local_pawn(0);
    API::UObject* mesh = shape();
    auto* cls = ue::find_class(L"Class /Script/Engine.StaticMeshComponent");
    if (pawn == nullptr || mesh == nullptr || cls == nullptr || !anchor(pawn)) {
        LOGW("%s cornermask: not built (pawn %d, a basic shape %d, the component class %d, the head anchor %d)", TAG, pawn != nullptr, mesh != nullptr, cls != nullptr, ue::alive(g_anchor));
        g_seen = FILETIME{};   // try again at the next check
        return;
    }
    std::wifstream f(p.c_str());
    std::wstring line;
    while (std::getline(f, line)) {
        const size_t hash = line.find(L'#');
        if (hash != std::wstring::npos) line.erase(hash);
        double v[9];
        if (swscanf_s(line.c_str(), L"%lf %lf %lf %lf %lf %lf %lf %lf %lf", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8]) != 9) continue;
        ue::Transform t;
        API::UObject* comp = nullptr;
        if (!ue::call_ret(pawn, L"AddComponentByClass", {(API::UObject*)cls, true, t, false}, comp) || comp == nullptr) continue;
        ue::call(comp, L"SetStaticMesh", {mesh});
        ue::call(comp, L"SetCollisionEnabled", {(int32_t)0});
        ue::call(comp, L"SetCastShadow", {false});
        bool attached = false;
        ue::call_ret(comp, L"K2_AttachTo", {g_anchor, ue::name(L""), (int32_t)0, false}, attached);
        ue::call(comp, L"K2_SetRelativeLocationAndRotation", {ue::Vec3{v[0], v[1], v[2]}, ue::Rot{v[3], v[4], v[5]}, false});
        ue::call(comp, L"SetRelativeScale3D", {ue::Vec3{v[6], v[7], v[8]}});
        ue::call(comp, L"SetVisibility", {true});
        ue::call(comp, L"SetHiddenInGame", {false});
        g_pieces.push_back(comp);
    }
    LOGI("%s cornermask: %zu piece(s) on the head, shape %s", TAG, g_pieces.size(), ue::name_of(mesh).c_str());
}
} // namespace

void tick(float delta) {
    g_since += delta;
    if (g_since < CHECK_S) return;
    g_since = 0.0f;
    const std::wstring p = path();
    const int c = changed(p);
    if (c > 0) build(p);
    else if (c < 0) clear();
}

} // namespace sh2vr::cornermask
