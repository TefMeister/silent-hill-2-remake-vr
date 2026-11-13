// torch.cpp -- see torch.h.
#include "torch.h"

#include <windows.h>

#include <cstring>
#include <string>

#include "common.h"
#include "settings.h"
#include "ue.h"
#include "uevr/API.hpp"

namespace sh2vr::torch {

namespace {
API::UObject* g_light = nullptr;
float g_since = 1.0f;
bool g_off = false;
double g_eye[2][3] = {};
unsigned g_eye_frame[2] = {0, 0};
unsigned g_frame = 0;
int g_said = 0;

bool kept_off() {
    const std::wstring p = appdata_dir() + L"\\" + settings::PROFILE_DIR + L"\\sh2vr_notorchfix.txt";
    return GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// pawn.Items.EquipmentActors -> the SHFlashlight -> LightMain (read in place: the array is the game's, never copied)
API::UObject* find_light() {
    API::UObject* pawn = API::get()->get_local_pawn(0);
    if (pawn == nullptr) return nullptr;
    auto** items = pawn->get_property_data<API::UObject*>(L"Items");
    if (items == nullptr || *items == nullptr) return nullptr;
    struct RawArray { API::UObject** data; int32_t count; int32_t capacity; };
    auto* arr = (*items)->get_property_data<RawArray>(L"EquipmentActors");
    if (arr == nullptr || arr->data == nullptr) return nullptr;
    static API::UClass* cls = API::get()->find_uobject<API::UClass>(L"Class /Script/SHProto.SHFlashlight");
    if (cls == nullptr) return nullptr;
    for (int32_t i = 0; i < arr->count; ++i) {
        API::UObject* a = arr->data[i];
        if (a == nullptr || !a->is_a(cls)) continue;
        auto** light = a->get_property_data<API::UObject*>(L"LightMain");
        return light != nullptr ? *light : nullptr;
    }
    return nullptr;
}
} // namespace

void tick(float delta) {
    if (!settings::TORCH_BETWEEN_EYES) return;
    ++g_frame;
    g_since += delta;
    if (g_since < 2.0f) return;
    g_since = 0.0f;
    g_off = kept_off();
    if (g_light != nullptr && ue::alive(g_light)) return;
    g_light = find_light();
    if (g_light != nullptr && g_said < 5) { ++g_said; LOGI("%s torch: the flashlight's main light found, it is put between the eyes%s", TAG, g_off ? " (NOT: sh2vr_notorchfix.txt)" : ""); }
}

void on_post_view(int view_index, void* position) {
    if (!settings::TORCH_BETWEEN_EYES || g_off || g_light == nullptr || position == nullptr) return;
    const int e = view_index & 1;
    std::memcpy(g_eye[e], position, sizeof g_eye[e]);   // FVector, three doubles (UE5)
    g_eye_frame[e] = g_frame;
    const int o = e ^ 1;
    if (g_eye_frame[o] == 0 || g_frame - g_eye_frame[o] > 2) return;   // the other eye not seen lately: leave it
    const ue::Vec3 mid{(g_eye[0][0] + g_eye[1][0]) * 0.5, (g_eye[0][1] + g_eye[1][1]) * 0.5, (g_eye[0][2] + g_eye[1][2]) * 0.5};
    ue::call(g_light, L"K2_SetWorldLocation", {mid, false});   // (location, sweep); the hit result and teleport stay zero
}

} // namespace sh2vr::torch
