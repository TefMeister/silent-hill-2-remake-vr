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
constexpr int RING = 32;   // one view a tick, the render thread a frame or two behind: ample
struct Rec { Entry e; double keyd[3]{}; };   // the record, and its place in doubles (the game's own origin)
std::mutex g_mtx;
Rec g_ring[RING];
uint64_t g_seq = 0;        // the newest record's number (0 = none yet)

std::atomic<float> g_world_scale{1.0f};
std::atomic<int> g_hand_ticks{settings::DRAWNPOSE_HAND_TICKS};   // the hands of the record this many ticks after the view's own (a test switch)
float g_scale_timer = 1.0f;

// THE WARP'S ORIGIN (found live 2026-11-18): in play the views the installed injector hands the warp are measured
// from another origin than the game's own (the first run: about 311 m and 92 m away, 1.02 m lower; in the main menu:
// none), a plain shift. It is learned here: record minus call is that shift for the right record, and it is the one
// difference that stays the same from one call to the next.
// All under g_mtx (written by the render thread's find()).
bool g_have_origin = false;
double g_origin[3]{};
int g_origin_misses = 0;                    // calls in a row no record fitted the origin: after a few it is learned again
struct Cand { double d[3]; uint64_t seq; };
Cand g_prev[RING]; int g_prev_n = 0;        // the last call's differences to every record
double g_pending[3]{}; int g_pending_n = 0; // a shift seen in consecutive calls, and how many times
uint64_t g_last_lag = 2;
int g_origin_learned = 0;

// counted for the log line
std::atomic<int> g_found{0}, g_missed{0}, g_tied{0}, g_by_drawn{0}, g_by_raw{0};
std::atomic<int64_t> g_lag_sum{0};
std::atomic<int64_t> g_differ_um_sum{0};
std::atomic<int> g_differ_um_max{0};

bool near3(const double a[3], const double b[3], double tol) { return std::fabs(a[0] - b[0]) < tol && std::fabs(a[1] - b[1]) < tol && std::fabs(a[2] - b[2]) < tol; }
bool live(const Rec& r) { return r.e.seq != 0 && r.e.seq + RING > g_seq; }

// learn the origin from this call's differences against the last call's: the shift both calls share
void learn(const Cand* cur, int n) {
    double found[3]{}; int kinds = 0;
    for (int i = 0; i < g_prev_n && kinds < 2; ++i) for (int j = 0; j < n && kinds < 2; ++j) {
        if (cur[j].seq <= g_prev[i].seq || !near3(g_prev[i].d, cur[j].d, settings::DRAWNPOSE_ORIGIN_TOL_M)) continue;
        if (kinds == 0) { for (int k = 0; k < 3; ++k) found[k] = cur[j].d[k]; kinds = 1; }
        else if (!near3(found, cur[j].d, settings::DRAWNPOSE_ORIGIN_TOL_M)) kinds = 2;   // two different shifts fit (an even glide): wait
    }
    if (kinds == 1 && (g_pending_n == 0 || near3(found, g_pending, settings::DRAWNPOSE_ORIGIN_TOL_M))) {
        for (int k = 0; k < 3; ++k) g_pending[k] = found[k];
        if (++g_pending_n >= settings::DRAWNPOSE_ORIGIN_CONFIRM) {
            for (int k = 0; k < 3; ++k) g_origin[k] = g_pending[k];
            g_have_origin = true; g_origin_misses = 0; g_pending_n = 0; ++g_origin_learned;
        }
    } else g_pending_n = 0;
    for (int j = 0; j < n; ++j) g_prev[j] = cur[j];
    g_prev_n = n;
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
    const float scale = world_to_meters * g_world_scale.load();   // as UEVR's open side: the float product, the division in doubles
    if (!(scale > 1e-3f)) return;
    const double* p = (const double*)position;
    Rec r;
    r.keyd[0] = p[1] / (double)scale; r.keyd[1] = p[2] / (double)scale; r.keyd[2] = -p[0] / (double)scale;
    for (int k = 0; k < 3; ++k) r.e.key[k] = (float)r.keyd[k];
    const double* rot = (const double*)rotation;   // FRotator: pitch, yaw, roll (degrees)
    const double yaw = -rot[1] * 0.017453292519943295, pitch = rot[0] * 0.017453292519943295;
    r.e.fwd[0] = (float)(std::sin(yaw) * std::cos(pitch)); r.e.fwd[1] = (float)(-std::sin(pitch)); r.e.fwd[2] = (float)(std::cos(yaw) * std::cos(pitch));
    for (int i = 0; i < 2; ++i) {
        ue::Vec3 pos; ue::Quat q;
        if (!ik::drawn(i == 0 ? controllers::LEFT : controllers::RIGHT, pos, q)) continue;
        const double pp[3] = {pos.x, pos.y, pos.z}, qq[4] = {q.x, q.y, q.z, q.w};
        r.e.hand[i] = carry_math::to_warp_world(pp, qq, (double)scale);
    }
    std::lock_guard<std::mutex> lock(g_mtx);
    r.e.seq = ++g_seq;
    g_ring[r.e.seq % RING] = r;
}

bool find(const float* v2w, Entry& out) {
    if (!settings::DRAWNPOSE_ON || v2w == nullptr) return false;
    const double key[3] = {v2w[12], v2w[13], v2w[14]};
    const float fwd[3] = {v2w[8], v2w[9], v2w[10]};
    std::lock_guard<std::mutex> lock(g_mtx);
    if (g_seq == 0) return false;
    // this call against every record: record minus call
    Cand cur[RING]; const Rec* rec[RING]; int n = 0;
    for (int i = 0; i < RING; ++i) {
        if (!live(g_ring[i])) continue;
        for (int k = 0; k < 3; ++k) cur[n].d[k] = g_ring[i].keyd[k] - key[k];
        cur[n].seq = g_ring[i].e.seq; rec[n] = &g_ring[i]; ++n;
    }
    const Rec* best = nullptr; int matches = 0;
    if (g_have_origin) {
        // the records at this place; several (a dead-still head) : the same turn first, then the usual distance behind
        uint64_t best_off = ~0ull;
        for (int pass = 0; pass < 2 && best == nullptr; ++pass) {
            matches = 0;
            for (int j = 0; j < n; ++j) {
                if (!near3(cur[j].d, g_origin, settings::DRAWNPOSE_ORIGIN_TOL_M)) continue;
                const Entry& e = rec[j]->e;
                if (pass == 0 && (std::fabs(e.fwd[0] - fwd[0]) > settings::DRAWNPOSE_TURN_TOL || std::fabs(e.fwd[1] - fwd[1]) > settings::DRAWNPOSE_TURN_TOL || std::fabs(e.fwd[2] - fwd[2]) > settings::DRAWNPOSE_TURN_TOL)) continue;
                ++matches;
                const uint64_t lag = g_seq - e.seq, off = lag > g_last_lag ? lag - g_last_lag : g_last_lag - lag;
                if (off < best_off) { best_off = off; best = rec[j]; }
            }
        }
        if (best == nullptr && ++g_origin_misses >= settings::DRAWNPOSE_ORIGIN_DROP) { g_have_origin = false; g_pending_n = 0; g_prev_n = 0; }
    }
    if (!g_have_origin) learn(cur, n); else { for (int j = 0; j < n; ++j) g_prev[j] = cur[j]; g_prev_n = n; }
    if (best == nullptr) { g_missed.fetch_add(1); return false; }
    g_origin_misses = 0;
    if (matches == 1) g_last_lag = g_seq - best->e.seq; else g_tied.fetch_add(1);
    out = best->e;
    if (const int shift = g_hand_ticks.load(); shift != 0) {   // the test switch: the hands of a neighbouring tick
        const uint64_t want = best->e.seq + (uint64_t)(int64_t)shift;
        const Rec& o = g_ring[want % RING];
        if (live(o) && o.e.seq == want) { out.hand[0] = o.e.hand[0]; out.hand[1] = o.e.hand[1]; }
        else { out.hand[0].ok = out.hand[1].ok = false; }
    }
    // the hands into the warp's own origin, like the views they are carried between
    for (int i = 0; i < 2; ++i) for (int k = 0; k < 3; ++k) out.hand[i].t[k] -= g_origin[k];
    g_found.fetch_add(1);
    g_lag_sum.fetch_add((int64_t)(g_seq - best->e.seq));
    return true;
}

void set_hand_ticks(int ticks) { g_hand_ticks.store(ticks); }

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
    double o[3]; bool have; int learned;
    { std::lock_guard<std::mutex> lock(g_mtx); have = g_have_origin; learned = g_origin_learned; for (int k = 0; k < 3; ++k) o[k] = g_origin[k]; }
    // the proof line: how many made-up hands followed the drawn hand, and how far off the raw controller's carry would have been
    LOGI("%s drawnpose: %d views matched to their frame (%d not, %d with several frames at one place), %.1f records behind; hands carried: %d by the drawn hand, %d by the raw controller; the raw carry differed by %.1f mm on average, %.1f mm at most; the warp's origin %s %.3f %.3f %.3f m (learned %d times)",
         TAG, found, missed, tied, found > 0 ? (double)lag / found : 0.0, drawn, raw, drawn > 0 ? (double)um / drawn / 1000.0 : 0.0, um_max / 1000.0,
         have ? "is at" : "NOT KNOWN, last", o[0], o[1], o[2], learned);
}

} // namespace sh2vr::drawnpose
