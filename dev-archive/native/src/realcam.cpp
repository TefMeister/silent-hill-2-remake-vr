// realcam.cpp -- see realcam.h.
// Which step runs last (the lever rule): the engine asks the stereo device for the projection AFTER the view offset,
// once per view, and uses what comes back as it is; the injector's hook sits INSIDE the function we call first, so
// our two numbers are written last. Proof in the log: "realcam: angle" lines carry the value before and after.
#include "realcam.h"

#include <windows.h>

#include <atomic>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "common.h"
#include "settings.h"
#include "ue.h"
#include "uevr/API.hpp"

namespace sh2vr::realcam {

namespace {
constexpr const char* LOG_MARK = "Found CalculateStereoProjectionMatrix: ";
constexpr double PI = 3.14159265358979323846;
// FCameraCacheEntry { float TimeStamp; FMinimalViewInfo POV @0x10 } and FMinimalViewInfo { FVector, FRotator, float FOV @0x30 }
// (Unreal 5 layout); checked live against GetFOVAngle() before it is used (tick below).
constexpr size_t POV_FOV_OFFSET = 0x10 + 0x30;
constexpr float FOV_MIN = 3.0f, FOV_MAX = 170.0f;
constexpr float RAW_CHECK_TOLERANCE = 0.05f;
constexpr float ANGLE_LOG_STEP = 0.5f;
constexpr int ANGLE_LOG_MAX = 60;

using ProjFn = double* (*)(void* self, double* out, int view);   // FMatrix (16 doubles) through the hidden return pointer
std::atomic<bool> g_active{false};
std::atomic<float> g_fov{0.0f};          // this frame's camera angle, horizontal degrees; 0 = not known
std::atomic<int> g_raw_ok{0};            // 0 not checked, 1 the raw read agrees with GetFOVAngle, -1 it does not
ProjFn g_orig = nullptr;
bool g_tried = false;
unsigned char g_kept[24];
bool g_have_kept = false;
int g_said = 0;
float g_last_logged = 0.0f;

API::UObject* camera_manager() {
    API::UObject* pc = API::get()->get_player_controller(0);
    if (pc == nullptr) return nullptr;
    API::UObject** pcm = ue::prop<API::UObject*>(pc, L"PlayerCameraManager");
    return pcm != nullptr ? *pcm : nullptr;
}

float raw_fov() {
    API::UObject* pcm = camera_manager();
    if (pcm == nullptr) return 0.0f;
    const unsigned char* cache = ue::prop<unsigned char>(pcm, L"CameraCachePrivate");
    if (cache == nullptr) return 0.0f;
    float f = 0.0f;
    std::memcpy(&f, cache + POV_FOV_OFFSET, sizeof f);
    return (f >= FOV_MIN && f <= FOV_MAX) ? f : 0.0f;
}

double* hooked(void* self, double* out, int view) {
    double* r = g_orig(self, out, view);
    const float fov = g_fov.load();
    if (!g_active.load() || fov <= 0.0f || out == nullptr) return r;
    const double before = out[0];
    if (before <= 0.0 || out[5] <= 0.0) return r;
    const double aspect = out[5] / out[0];                 // the injector's own width / height for the flat picture
    const double t = std::tan((double)fov * PI / 360.0);
    out[0] = 1.0 / t;                                      // M[0][0]
    out[5] = aspect / t;                                   // M[1][1]
    out[8] = 0.0; out[9] = 0.0;                            // M[2][0], M[2][1]: a centred picture
    if (view == 0 && g_said < ANGLE_LOG_MAX && std::fabs(fov - g_last_logged) > ANGLE_LOG_STEP) {
        ++g_said; g_last_logged = fov;
        LOGI("%s realcam: angle %.1f degrees, aspect %.3f: the projection's first number %.3f -> %.3f", TAG, fov, aspect, before, out[0]);
    }
    return r;
}

// The entry the injector found at its own start; ours goes in only when the table really holds that address there.
void install(void* device) {
    g_tried = true;
    std::ifstream f(appdata_dir() + L"\\" + settings::PROFILE_DIR + L"\\log.txt");
    std::string line, found;
    while (std::getline(f, line)) {
        const auto p = line.find(LOG_MARK);
        if (p != std::string::npos) found = line.substr(p + std::strlen(LOG_MARK));
    }
    int index = -1; unsigned long long addr = 0;
    std::istringstream is(found);
    is >> index >> std::hex >> addr;
    if (index < 1 || index > 60 || addr == 0) { LOGI("%s realcam: NOT installed: the injector's log does not name the projection entry (\"%s\")", TAG, found.c_str()); return; }
    void** table = *(void***)device;
    if ((unsigned long long)table[index] != addr) { LOGI("%s realcam: NOT installed: entry %d holds %p, the log says %llx", TAG, index, table[index], addr); return; }
    DWORD old = 0;
    if (!VirtualProtect(&table[index], sizeof(void*), PAGE_READWRITE, &old)) { LOGI("%s realcam: NOT installed: the table cannot be written", TAG); return; }
    g_orig = (ProjFn)table[index];
    table[index] = (void*)&hooked;
    VirtualProtect(&table[index], sizeof(void*), old, &old);
    LOGI("%s realcam: installed on the stereo device's entry %d (%llx)", TAG, index, addr);
}
} // namespace

void set_active(bool on) {
    g_active.store(on);
    LOGI("%s realcam: %s", TAG, on ? "ON (the cutscene camera's own angle, one picture for both eyes)" : "off");
}

void on_pre_view(void* device, void* position) {
    if (!settings::CUTSCENE_REAL_CAMERA) return;
    if (!g_tried && device != nullptr) install(device);
    g_have_kept = false;
    if (!g_active.load() || position == nullptr) return;
    std::memcpy(g_kept, position, sizeof g_kept);          // FVector, three doubles
    g_have_kept = true;
    if (g_raw_ok.load() == 1) { const float f = raw_fov(); if (f > 0.0f) g_fov.store(f); }
}

void on_post_view(void* position) {
    if (g_have_kept && position != nullptr) std::memcpy(position, g_kept, sizeof g_kept);
}

void tick(float delta) {
    if (!settings::CUTSCENE_REAL_CAMERA) return;
    static float since = 0.0f;
    since += delta;
    const int state = g_raw_ok.load();
    if (since < (state == 1 ? 5.0f : 0.05f)) return;
    since = 0.0f;
    API::UObject* pcm = camera_manager();
    if (pcm == nullptr) return;
    float asked = 0.0f;
    if (!ue::call_ret(pcm, L"GetFOVAngle", {}, asked)) return;
    const float raw = raw_fov();
    if (state == 0) {
        const bool same = raw > 0.0f && std::fabs(raw - asked) < RAW_CHECK_TOLERANCE;
        g_raw_ok.store(same ? 1 : -1);
        LOGI("%s realcam: the camera's angle read raw %.2f, asked %.2f: %s", TAG, raw, asked, same ? "the raw read is used (same frame)" : "they differ, the asked value is used (up to a frame late)");
    }
    if (g_raw_ok.load() != 1 && asked >= FOV_MIN && asked <= FOV_MAX) g_fov.store(asked);
}

} // namespace sh2vr::realcam
