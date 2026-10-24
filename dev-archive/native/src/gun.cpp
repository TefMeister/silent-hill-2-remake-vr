// gun.cpp -- see gun.h
#include "gun.h"

#include "settings.h"

namespace sh2vr::gun {

namespace {
API::UObject* kml() { return ue::cdo(L"Class /Script/Engine.KismetMathLibrary"); }
} // namespace

API::UObject* pawn() { return API::get()->get_local_pawn(0); }

API::UObject* ranged_part() {
    auto** c = ue::prop<API::UObject*>(pawn(), L"Combat");
    auto* cls = ue::find_class(L"Class /Script/SHProto.SHRangedCmbSubcomp");
    API::UObject* sub = nullptr;
    if (c == nullptr || *c == nullptr || cls == nullptr || !ue::call_ret(*c, L"FindSubcomponentByClass", {(API::UObject*)cls}, sub)) return nullptr;
    return sub;
}

API::UObject* equipped() {
    API::UObject* sub = ranged_part();
    API::UObject* w = nullptr;
    if (sub != nullptr) ue::call_ret(sub, L"GetEquippedRangedWeapon", {}, w);
    return ue::alive(w) ? w : nullptr;
}

bool is_pistol(API::UObject* weapon) {
    if (!ue::alive(weapon)) return false;
    return ue::name_of((API::UObject*)weapon->get_class()).find(narrow(settings::PISTOL_CLASS)) != std::string::npos;
}

API::UObject* mesh(API::UObject* weapon) {
    auto** m = ue::prop<API::UObject*>(weapon, L"Mesh");
    return m != nullptr && ue::alive(*m) ? *m : nullptr;
}

bool socket_loc(API::UObject* m, std::wstring_view socket, Vec3& out) {
    return m != nullptr && ue::call_ret(m, L"GetSocketLocation", {ue::name(socket)}, out);
}

bool socket_xform(API::UObject* m, std::wstring_view socket, Transform& out) {   // ERelativeTransformSpace::RTS_World = 0
    return m != nullptr && ue::call_ret(m, L"GetSocketTransform", {ue::name(socket), (int32_t)0}, out);
}

bool controller(controllers::Side side, Vec3& loc, Rot& rot) {
    API::UObject* c = controllers::find(side);
    return c != nullptr && ue::call_ret(c, L"K2_GetComponentLocation", {}, loc) && ue::call_ret(c, L"K2_GetComponentRotation", {}, rot);
}

bool hmd(Vec3& loc, Rot& rot) {
    API::UObject* pc = API::get()->get_player_controller(0);
    auto** cam = ue::prop<API::UObject*>(pc, L"PlayerCameraManager");
    if (cam == nullptr || !ue::alive(*cam)) return false;
    return ue::call_ret(*cam, L"GetCameraLocation", {}, loc) && ue::call_ret(*cam, L"GetCameraRotation", {}, rot);
}

bool hmd_active() {
    const auto* vr = API::get()->param()->vr;
    return vr != nullptr && vr->is_hmd_active();
}

void buzz(controllers::Side side, float amplitude, float seconds) {
    const UEVR_InputSourceHandle src = side == controllers::LEFT ? API::VR::get_left_joystick_source() : API::VR::get_right_joystick_source();
    API::VR::trigger_haptic_vibration(0.0f, amplitude, settings::BUZZ_HZ, seconds, src);
}

Rot rot_of(const Transform& t) {
    Rot r;
    ue::call_ret(kml(), L"TransformRotation", {t, Rot{}}, r);
    return r;
}

} // namespace sh2vr::gun
