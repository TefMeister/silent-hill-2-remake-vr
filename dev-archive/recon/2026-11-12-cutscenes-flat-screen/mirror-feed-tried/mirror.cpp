// mirror.cpp -- see mirror.h. [compile-verified 2026-11-12] until it has run.
#include "mirror.h"

#include <cmath>
#include <mutex>
#include <vector>

#include "common.h"
#include "settings.h"
#include "ue.h"
#include "uevr/API.hpp"

namespace sh2vr::mirror {

namespace {
constexpr float RAD_TO_DEG = 57.29577951f;
std::mutex g_mu;
ue::Vec3 g_pos;            // the left eye's view, as drawn
ue::Rot g_rot;
bool g_have = false;
std::vector<API::UObject*> g_mirrors;
float g_since_list = settings::MIRROR_LIST_EVERY_S;   // the first tick lists at once
int g_said = 0;

bool set_bool(API::UObject* o, const wchar_t* name, bool v) {
    auto* p = o->get_class()->find_property(name);
    if (p == nullptr) return false;
    static_cast<API::FBoolProperty*>(p)->set_value_in_object(o, v);
    return true;
}

// the horizontal angle the frame is drawn with: the left eye's projection, as one symmetric angle
float drawn_fov_deg() {
    const auto m = API::VR::get_ue_projection_matrix(API::VR::Eye::LEFT);
    const float m00 = m.m[0][0];
    if (!(m00 > 0.01f)) return 0.0f;
    return 2.0f * std::atan(1.0f / m00) * RAD_TO_DEG;
}
} // namespace

void on_view(int view_index, const void* position, const void* rotation, bool is_double) {
    if (!settings::MIRROR_FIX || view_index != 0 || position == nullptr || rotation == nullptr) return;
    std::lock_guard<std::mutex> lk(g_mu);
    if (is_double) {
        const auto* p = static_cast<const UEVR_Vector3d*>(position);
        const auto* r = static_cast<const UEVR_Rotatord*>(rotation);
        g_pos = {p->x, p->y, p->z};
        g_rot = {r->pitch, r->yaw, r->roll};
    } else {
        const auto* p = static_cast<const UEVR_Vector3f*>(position);
        const auto* r = static_cast<const UEVR_Rotatorf*>(rotation);
        g_pos = {p->x, p->y, p->z};
        g_rot = {r->pitch, r->yaw, r->roll};
    }
    g_have = true;
}

void tick(float delta) {
    if (!settings::MIRROR_FIX) return;
    g_since_list += delta;
    if (g_since_list >= settings::MIRROR_LIST_EVERY_S) {
        g_since_list = 0.0f;
        g_mirrors.clear();
        if (auto* cls = ue::find_class(L"Class /Script/SHProto.SHFakeMirrorComponent")) {
            for (auto* o : cls->get_objects_matching<API::UObject>(false)) g_mirrors.push_back(o);
        }
    }
    if (g_mirrors.empty()) return;
    ue::Vec3 pos; ue::Rot rot;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        if (!g_have) return;
        pos = g_pos; rot = g_rot;
    }
    const float fov = drawn_fov_deg() * settings::MIRROR_FOV_SCALE;
    if (fov < 5.0f || fov > 170.0f) return;
    int fed = 0;
    for (auto* m : g_mirrors) {
        if (!ue::alive(m)) continue;
        auto* p = ue::prop<ue::Vec3>(m, L"FakeCameraPosition");
        auto* r = ue::prop<ue::Rot>(m, L"FakeCameraRotation");
        auto* f = ue::prop<float>(m, L"FakeCameraFOV");
        if (p == nullptr || r == nullptr || f == nullptr) continue;
        *p = pos; *r = rot; *f = fov;
        if (set_bool(m, L"AllowFakeInputs", true)) ++fed;
    }
    if (fed > 0 && g_said < 5) {
        ++g_said;
        LOGI("%s mirror: %d fake mirror(s) fed the drawn view: position %.0f %.0f %.0f, pitch %.1f yaw %.1f roll %.1f, angle %.1f", TAG, fed,
             pos.x, pos.y, pos.z, rot.pitch, rot.yaw, rot.roll, (double)fov);
    }
}

} // namespace sh2vr::mirror
