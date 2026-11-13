// warpscale.cpp -- see warpscale.h.
#include "warpscale.h"

#include <windows.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "common.h"
#include "settings.h"
#include "uevr/API.hpp"

namespace sh2vr::warpscale {

namespace {
// The layout of the call's argument, from the public header of the warp (PureDark's UEVR fork, pd-afwmod/include):
// only the three fields used here. Checked at run time before anything is touched (a newer warp may differ).
constexpr size_t OFF_MODE = 60;          // FrameWarpMode, 0..3
constexpr size_t OFF_CAMERA = 72;        // CameraData*: 12 matrices of 16 floats, column-major (glm)
constexpr int MATS = 12, MAT_FLOATS = 16;
constexpr int SETS = 2;                  // dest and src (the third set, the UI camera, is not used by the injector)
// within a set: world-to-view, view-to-world, view-to-clip, clip-to-view
constexpr int M_W2V = 0, M_V2W = 1, M_V2C = 2, M_C2V = 3;

using EvaluateFn = void(__stdcall*)(void*);
EvaluateFn g_real = nullptr;
void** g_slot = nullptr;
std::atomic<float> g_scale{settings::WARP_NEAR_SCALE};
std::atomic<int> g_calls{0}, g_scaled{0}, g_refused{0}, g_combined{0}, g_switches{0};
std::atomic<float> g_turn_deg{settings::WARP_TURN_DEG_PER_FRAME};   // 0 = the mode is never touched
std::atomic<float> g_hand_speed{settings::WARP_HAND_M_PER_S};       // 0 = the hands' speed is not looked at
std::atomic<bool> g_hands_fast{false};                              // set by the game thread, read by the hooked call
// the render thread's own (only the hooked call reads and writes these)
float g_last_rot[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
int g_hold = 0;
bool g_was_combined = false;
constexpr int32_t MODE_PREVIOUS = 2, MODE_COMBINED = 3;

// the angle, in degrees, between two rotations given as the 3x3 corner of a view matrix
float angle_between(const float* a16, const float* b9) {
    float tr = 0.0f;
    for (int c = 0; c < 3; ++c) for (int r = 0; r < 3; ++r) tr += a16[c * 4 + r] * b9[c * 3 + r];
    float cosv = (tr - 1.0f) * 0.5f;
    cosv = cosv > 1.0f ? 1.0f : cosv < -1.0f ? -1.0f : cosv;
    return std::acos(cosv) * 57.29578f;
}

// THE MODE PER FRAME: the previous-frame mode (2) has clean edges but leaves the hands behind when the view turns; the
// combined mode (3) carries them. While the view turns faster than the threshold, a call that asked for 2 is made as 3.
void pick_mode(uint8_t* bytes, const float* cam) {
    const float limit = g_turn_deg.load();
    int32_t* mode = (int32_t*)(bytes + OFF_MODE);
    const float* w2v = cam + (4 + M_W2V) * MAT_FLOATS;   // the drawn eye's view, this frame
    const float turned = angle_between(w2v, g_last_rot);
    for (int c = 0; c < 3; ++c) for (int r = 0; r < 3; ++r) g_last_rot[c * 3 + r] = w2v[c * 4 + r];
    if (limit <= 0.0f || *mode != MODE_PREVIOUS) { g_hold = 0; g_was_combined = false; return; }
    if ((turned > limit && turned < 45.0f) || g_hands_fast.load()) g_hold = settings::WARP_TURN_HOLD_FRAMES; else if (g_hold > 0) --g_hold;
    const bool combined = g_hold > 0;
    if (combined != g_was_combined) { g_was_combined = combined; g_switches.fetch_add(1); }
    if (combined) { *mode = MODE_COMBINED; g_combined.fetch_add(1); }
}
float g_file_timer = 0.0f, g_report_timer = 0.0f;
bool g_gave_up = false, g_first_logged = false;

bool readable(const void* p, size_t n) {
    MEMORY_BASIC_INFORMATION mbi{};
    if (p == nullptr || VirtualQuery(p, &mbi, sizeof(mbi)) == 0) return false;
    if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) return false;
    return (const uint8_t*)p + n <= (const uint8_t*)mbi.BaseAddress + mbi.RegionSize;
}

// an affine world/view matrix as glm stores it: last row 0 0 0 1 (indices 3, 7, 11, 15)
bool affine(const float* m) {
    return std::fabs(m[3]) < 1e-4f && std::fabs(m[7]) < 1e-4f && std::fabs(m[11]) < 1e-4f && std::fabs(m[15] - 1.0f) < 1e-4f;
}
// a perspective matrix: w comes from view z only (indices 3, 7 and 15 zero, 11 not)
bool perspective(const float* m) {
    return std::fabs(m[3]) < 1e-4f && std::fabs(m[7]) < 1e-4f && std::fabs(m[15]) < 1e-4f && std::fabs(m[11]) > 0.5f;
}

void __stdcall hooked(void* params) {
    const float k = g_scale.load();
    g_calls.fetch_add(1);
    if ((k == 1.0f && g_turn_deg.load() <= 0.0f) || params == nullptr || !readable(params, OFF_CAMERA + sizeof(void*))) { g_real(params); return; }
    auto* bytes = (uint8_t*)params;
    const int32_t asked = *(int32_t*)(bytes + OFF_MODE);
    float* cam = *(float**)(bytes + OFF_CAMERA);
    bool ok = asked >= 0 && asked <= 3 && readable(cam, MATS * MAT_FLOATS * sizeof(float));
    for (int s = 0; ok && s < SETS; ++s) {
        const float* set = cam + s * 4 * MAT_FLOATS;
        ok = affine(set + M_W2V * MAT_FLOATS) && affine(set + M_V2W * MAT_FLOATS) && perspective(set + M_V2C * MAT_FLOATS);
    }
    if (!ok) { g_refused.fetch_add(1); g_real(params); return; }

    pick_mode(bytes, cam);
    float copy[MATS * MAT_FLOATS];
    if (k != 1.0f) {
        std::memcpy(copy, cam, sizeof(copy));
        for (int s = 0; s < SETS; ++s) {
            float* set = copy + s * 4 * MAT_FLOATS;
            for (int m : {M_W2V, M_V2W}) for (int i = 12; i <= 14; ++i) set[m * MAT_FLOATS + i] *= k;   // positions k times further
            for (int i = 12; i <= 15; ++i) set[M_V2C * MAT_FLOATS + i] *= k;                             // the near plane k times further
            for (int i : {3, 7, 11, 15}) set[M_C2V * MAT_FLOATS + i] /= k;                               // and its inverse
        }
        *(float**)(bytes + OFF_CAMERA) = copy;
        g_scaled.fetch_add(1);
    }
    g_real(params);
    *(float**)(bytes + OFF_CAMERA) = cam;
    *(int32_t*)(bytes + OFF_MODE) = asked;
}

// the injector reaches the warp through a delay-load import: find that table's slot for EvaluateFrameWarp
void** find_slot(HMODULE backend) {
    auto* base = (uint8_t*)backend;
    auto* dos = (IMAGE_DOS_HEADER*)base;
    auto* nt = (IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
    const auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
    if (dir.VirtualAddress == 0) return nullptr;
    for (auto* d = (IMAGE_DELAYLOAD_DESCRIPTOR*)(base + dir.VirtualAddress); d->DllNameRVA != 0; ++d) {
        if (_stricmp((const char*)(base + d->DllNameRVA), "PDAFWPlugin.dll") != 0) continue;
        auto* names = (IMAGE_THUNK_DATA64*)(base + d->ImportNameTableRVA);
        auto* slots = (void**)(base + d->ImportAddressTableRVA);
        for (int i = 0; names[i].u1.AddressOfData != 0; ++i) {
            if (IMAGE_SNAP_BY_ORDINAL64(names[i].u1.Ordinal)) continue;
            auto* by_name = (IMAGE_IMPORT_BY_NAME*)(base + names[i].u1.AddressOfData);
            if (std::strcmp(by_name->Name, "EvaluateFrameWarp") == 0) return &slots[i];
        }
    }
    return nullptr;
}

void install() {
    HMODULE backend = GetModuleHandleW(L"UEVRBackend.dll");
    HMODULE warp = GetModuleHandleW(L"PDAFWPlugin.dll");
    if (backend == nullptr || warp == nullptr) return;   // the warp's file is loaded at the first warped frame: ask again
    auto* real = (EvaluateFn)GetProcAddress(warp, "EvaluateFrameWarp");
    void** slot = find_slot(backend);
    if (real == nullptr || slot == nullptr) { g_gave_up = true; LOGW("%s warpscale: the injector's link to the warp was not found, left alone", TAG); return; }
    if (*slot != (void*)real) return;   // not resolved yet (still the loader's stub): ask again after the first call
    DWORD old = 0;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &old)) { g_gave_up = true; LOGW("%s warpscale: the link could not be written, left alone", TAG); return; }
    g_real = real;
    *slot = (void*)&hooked;
    VirtualProtect(slot, sizeof(void*), old, &old);
    g_slot = slot;
    LOGI("%s warpscale: standing in the injector's call to the frame warp; the scene is described %.2f times larger to it", TAG, g_scale.load());
}

void read_file() {
    const std::wstring p = appdata_dir() + L"\\" + settings::PROFILE_DIR + L"\\sh2vr_warpscale.txt";
    float v = settings::WARP_NEAR_SCALE, t = settings::WARP_TURN_DEG_PER_FRAME, h = settings::WARP_HAND_M_PER_S;
    FILE* f = nullptr;
    if (_wfopen_s(&f, p.c_str(), L"r") == 0 && f != nullptr) {
        float x = 0.0f, y = -1.0f, z = -1.0f;
        const int n = fscanf_s(f, "%f %f %f", &x, &y, &z);
        if (n >= 3 && z >= 0.0f && z <= 100.0f) h = z;
        if (n >= 1 && x >= settings::WARP_NEAR_SCALE_MIN && x <= settings::WARP_NEAR_SCALE_MAX) v = x;
        if (n >= 2 && y >= 0.0f && y <= 45.0f) t = y;
        std::fclose(f);
    }
    if (t != g_turn_deg.load()) { LOGI("%s warpscale: mode 3 while the view turns more than %.2f degrees a frame (was %.2f; 0 = never)", TAG, t, g_turn_deg.load()); g_turn_deg.store(t); }
    if (h != g_hand_speed.load()) { LOGI("%s warpscale: mode 3 while a hand moves faster than %.2f m/s in the room (was %.2f; 0 = never)", TAG, h, g_hand_speed.load()); g_hand_speed.store(h); }
    if (v != g_scale.load()) { LOGI("%s warpscale: factor %.2f -> %.2f", TAG, g_scale.load(), v); g_scale.store(v); }
}
} // namespace

// a hand moving through the room is left a frame behind by mode 2 just like a turn leaves it: measured here, on the game thread
void watch_hands(float delta) {
    static UEVR_Vector3f last[2]{};
    static bool have = false;
    const float limit = g_hand_speed.load();
    if (limit <= 0.0f || delta <= 0.0f) { g_hands_fast.store(false); have = false; return; }
    const UEVR_TrackedDeviceIndex idx[2] = {API::VR::get_left_controller_index(), API::VR::get_right_controller_index()};
    bool fast = false;
    for (int i = 0; i < 2; ++i) {
        const auto p = API::VR::get_pose(idx[i]).position;
        if (have) {
            const float dx = p.x - last[i].x, dy = p.y - last[i].y, dz = p.z - last[i].z;
            const float speed = std::sqrt(dx * dx + dy * dy + dz * dz) / delta;
            if (speed > limit && speed < settings::WARP_HAND_SPEED_GLITCH) fast = true;   // faster than a hand can move = a tracking jump
        }
        last[i] = p;
    }
    have = true;
    g_hands_fast.store(fast);
}

void tick(float delta) {
    if (!settings::WARP_NEAR_SCALE_ON || g_gave_up) return;
    watch_hands(delta);
    g_file_timer += delta;
    if (g_file_timer >= 1.0f) {
        g_file_timer = 0.0f;
        read_file();
        if (g_slot == nullptr) install();
    }
    if (g_slot == nullptr) return;
    g_report_timer += delta;
    if (g_report_timer >= settings::WARP_REPORT_S) {
        g_report_timer = 0.0f;
        const int calls = g_calls.exchange(0), comb = g_combined.exchange(0), sw = g_switches.exchange(0);
        if (!g_first_logged || comb > 0 || g_refused.load() > 0) {
            g_first_logged = true;
            LOGI("%s warpscale: %d warp calls, %d made in mode 3 for a turn (%d switches), %d scaled, %d left alone (unexpected camera data)", TAG,
                 calls, comb, sw, g_scaled.exchange(0), g_refused.exchange(0));
        }
    }
}

} // namespace sh2vr::warpscale
