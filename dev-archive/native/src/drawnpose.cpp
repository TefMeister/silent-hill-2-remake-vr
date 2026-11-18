// drawnpose.cpp -- see drawnpose.h.
#include "drawnpose.h"

#include <atomic>
#include <cmath>
#include <mutex>

#include "common.h"
#include "controllers.h"
#include "ik.h"
#include "settings.h"
#include "uevr/API.hpp"

namespace sh2vr::drawnpose {

namespace {
constexpr int RING = 32;   // a view or two a tick, the render thread a frame or two behind: ample
std::mutex g_mtx;
Entry g_ring[RING];
uint64_t g_seq = 0;        // the newest record's number (0 = none yet)

std::atomic<float> g_world_scale{1.0f};
float g_scale_timer = 1.0f;

// the render thread's own
uint64_t g_last_lag = 1;
// counted for the log line
std::atomic<int> g_found{0}, g_missed{0}, g_tied{0}, g_by_drawn{0}, g_by_raw{0};
std::atomic<int64_t> g_lag_sum{0};
std::atomic<int64_t> g_differ_um_sum{0};
std::atomic<int> g_differ_um_max{0};

bool same(const float a[3], const float b[3]) {
    for (int i = 0; i < 3; ++i) if (std::fabs(a[i] - b[i]) > settings::DRAWNPOSE_KEY_TOL_M * std::fmax(1.0f, std::fabs(b[i]))) return false;
    return true;
}
} // namespace

void tick(float delta) {
    g_scale_timer += delta;
    if (g_scale_timer < 1.0f) return;
    g_scale_timer = 0.0f;
    const float s = API::VR::get_mod_value<float>("VR_WorldScale");
    if (s > 0.001f && s < 1000.0f) g_world_scale.store(s);
}

void on_view(const void* position, const void* rotation, float world_to_meters, bool is_double) {
    if (!settings::DRAWNPOSE_ON || position == nullptr || rotation == nullptr || !is_double) return;
    // exactly as UEVR's open side does it: the float product, the division in doubles, then floats
    const float scale = world_to_meters * g_world_scale.load();
    if (!(scale > 1e-3f)) return;
    const double* p = (const double*)position;
    Entry e;
    e.key[0] = (float)(p[1] / (double)scale); e.key[1] = (float)(p[2] / (double)scale); e.key[2] = (float)(-p[0] / (double)scale);
    const double* r = (const double*)rotation;   // FRotator: pitch, yaw, roll (degrees)
    const double yaw = -r[1] * 0.017453292519943295, pitch = r[0] * 0.017453292519943295;
    e.fwd[0] = (float)(std::sin(yaw) * std::cos(pitch)); e.fwd[1] = (float)(-std::sin(pitch)); e.fwd[2] = (float)(std::cos(yaw) * std::cos(pitch));
    for (int i = 0; i < 2; ++i) {
        ue::Vec3 pos; ue::Quat rot;
        if (!ik::drawn(i == 0 ? controllers::LEFT : controllers::RIGHT, pos, rot)) continue;
        const double pp[3] = {pos.x, pos.y, pos.z}, q[4] = {rot.x, rot.y, rot.z, rot.w};
        e.hand[i] = carry_math::to_warp_world(pp, q, (double)scale);
    }
    std::lock_guard<std::mutex> lock(g_mtx);
    e.seq = ++g_seq;
    g_ring[e.seq % RING] = e;
}

bool find(const float* v2w, Entry& out) {
    if (!settings::DRAWNPOSE_ON || v2w == nullptr) return false;
    const float key[3] = {v2w[12], v2w[13], v2w[14]}, fwd[3] = {v2w[8], v2w[9], v2w[10]};
    std::lock_guard<std::mutex> lock(g_mtx);
    if (g_seq == 0) return false;
    // every record of this place; with the head moving there is exactly one. With the head dead still (the simulator)
    // several frames share a place: take the one as far behind the newest as the last sure match was.
    const Entry* best = nullptr; int matches = 0; uint64_t best_off = ~0ull;
    for (int pass = 0; pass < 2 && best == nullptr; ++pass) {   // first the records with the same place AND turn, then the place alone
        matches = 0;
        for (int i = 0; i < RING; ++i) {
            const Entry& e = g_ring[i];
            if (e.seq == 0 || e.seq + RING <= g_seq || !same(e.key, key)) continue;
            if (pass == 0 && (std::fabs(e.fwd[0] - fwd[0]) > settings::DRAWNPOSE_TURN_TOL || std::fabs(e.fwd[1] - fwd[1]) > settings::DRAWNPOSE_TURN_TOL || std::fabs(e.fwd[2] - fwd[2]) > settings::DRAWNPOSE_TURN_TOL)) continue;
            ++matches;
            const uint64_t lag = g_seq - e.seq, off = lag > g_last_lag ? lag - g_last_lag : g_last_lag - lag;
            if (off < best_off) { best_off = off; best = &e; }
        }
    }
    if (best == nullptr) { g_missed.fetch_add(1); return false; }
    if (matches == 1) g_last_lag = g_seq - best->seq; else g_tied.fetch_add(1);
    out = *best;
    g_found.fetch_add(1);
    g_lag_sum.fetch_add((int64_t)(g_seq - best->seq));
    return true;
}

void note(bool by_drawn, float differ_m) {
    if (!by_drawn) { g_by_raw.fetch_add(1); return; }
    g_by_drawn.fetch_add(1);
    if (differ_m >= 0.0f) {
        const int um = (int)(differ_m * 1e6f);
        g_differ_um_sum.fetch_add(um);
        int prev = g_differ_um_max.load();
        while (um > prev && !g_differ_um_max.compare_exchange_weak(prev, um)) {}
    }
}

void report() {
    const int found = g_found.exchange(0), missed = g_missed.exchange(0), tied = g_tied.exchange(0), drawn = g_by_drawn.exchange(0), raw = g_by_raw.exchange(0);
    const int64_t lag = g_lag_sum.exchange(0), um = g_differ_um_sum.exchange(0);
    const int um_max = g_differ_um_max.exchange(0);
    if (found + missed + drawn + raw == 0) return;
    // the proof line: how many made-up hands followed the drawn hand, and how far off the raw controller's carry would have been
    LOGI("%s drawnpose: %d views matched to their frame (%d not, %d with the head dead still), %.1f records behind; hands carried: %d by the drawn hand, %d by the raw controller; the raw carry differed by %.1f mm on average, %.1f mm at most",
         TAG, found, missed, tied, found > 0 ? (double)lag / found : 0.0, drawn, raw, drawn > 0 ? (double)um / drawn / 1000.0 : 0.0, um_max / 1000.0);
}

} // namespace sh2vr::drawnpose
