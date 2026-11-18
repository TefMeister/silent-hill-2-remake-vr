// carry_math.h -- the numbers behind "the made-up frame's hands follow the hands THE GAME DREW" (2026-11-18), pure
// maths and nothing of the game's or the card's, so a test can run the shipped code (tests/carry_test.cpp).
//
// The frame warp's call hands over each eye's view as UEVR's open side builds it (PureDark's public fork,
// FFakeStereoRenderingHook.cpp, right after the plugins' on_post_calculate_stereo_view_offset):
//     position = (UE view location) / (world to metres * world scale), axes (UE y, UE z, -UE x)
//     view-to-world = translate(position) * yawPitchRoll(-yaw, pitch, -roll)
// So a hand the game drew at a UE world place and turn can be put into that same space exactly (to_warp_world), and a
// pixel of the hand in an eye's LAST REAL picture can be carried to where the hand is in the frame being made up:
//     view then -> world -> (the hand's move from then to now) -> world -> view now          (carry)
// with both hand poses the ones the arm solve really used for those two frames. No controller reading, no guess of
// the timing: the smoothed gun, a hand riding a slide or a pump, and James walking are all in the two poses.
#pragma once

#include <cmath>

namespace sh2vr::carry_math {

struct Pose { bool ok = false; double R[9]{1, 0, 0, 0, 1, 0, 0, 0, 1}; double t[3]{}; };   // row-major turn, place (metres), the warp's world

// UE vector -> the warp's axes
inline void to_axes(const double v[3], double out[3]) { out[0] = v[1]; out[1] = v[2]; out[2] = -v[0]; }

// a UE world place (cm) and turn (quaternion x y z w) -> the warp's world; `scale` = UE units a metre
inline Pose to_warp_world(const double pos[3], const double q[4], double scale) {
    Pose p;
    if (!(scale > 1e-6)) return p;
    const double x = q[0], y = q[1], z = q[2], w = q[3];
    // the UE rotation matrix (columns = where the UE x, y, z axes go)
    const double ue[9] = {1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w),
                          2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w),
                          2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)};
    // P R P^T with P = rows (0 1 0), (0 0 1), (-1 0 0): row i of P picks UE row a[i] with sign s[i]
    const int a[3] = {1, 2, 0}; const double s[3] = {1, 1, -1};
    for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) p.R[i * 3 + j] = s[i] * s[j] * ue[a[i] * 3 + a[j]];
    double t[3]; to_axes(pos, t);
    for (int i = 0; i < 3; ++i) p.t[i] = t[i] / scale;
    p.ok = true;
    return p;
}

// glm matrices as the warp's call holds them: m[column * 4 + row], affine
inline double at(const float* m, int row, int col) { return (double)m[col * 4 + row]; }

// The carry: a point in front of the eye THEN (view space of w2v_then) -> where it is in front of the eye NOW
// (view space of w2v_now), for a thing rigidly on a hand that went from `then` to `now`.
// out_rows: 3 rows of (x y z | shift), the layout handpaste's pass reads. then_view: the hand's own place then.
// Returns false when a pose is missing.
inline bool carry(const float* w2v_then, const float* w2v_now, const Pose& then, const Pose& now, float out_rows[12], float then_view[3]) {
    if (!then.ok || !now.ok) return false;
    // view then -> world: x_w = A^T (x_v - a)   (A = the turn of w2v_then, a = its shift)
    double A[9], a[3], B[9], b[3];
    for (int r = 0; r < 3; ++r) { for (int c = 0; c < 3; ++c) { A[r * 3 + c] = at(w2v_then, r, c); B[r * 3 + c] = at(w2v_now, r, c); } a[r] = at(w2v_then, r, 3); b[r] = at(w2v_now, r, 3); }
    // the hand's move in the world: x' = D (x - t0) + t1, D = R1 R0^T
    double D[9]{};
    for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) for (int k = 0; k < 3; ++k) D[r * 3 + c] += now.R[r * 3 + k] * then.R[c * 3 + k];
    // x_now = B (D (A^T (x - a) - t0) + t1) + b  =  (B D A^T) x + B (t1 - D (A^T a + t0)) + b
    double DA[9]{}, M[9]{};
    for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) for (int k = 0; k < 3; ++k) DA[r * 3 + c] += D[r * 3 + k] * A[c * 3 + k];
    for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) for (int k = 0; k < 3; ++k) M[r * 3 + c] += B[r * 3 + k] * DA[k * 3 + c];
    double cam_then[3]{}, u[3], Du[3]{}, v[3], shift[3];   // cam_then = -A^T a: the eye's place then
    for (int r = 0; r < 3; ++r) for (int k = 0; k < 3; ++k) cam_then[r] -= A[k * 3 + r] * a[k];
    for (int r = 0; r < 3; ++r) u[r] = cam_then[r] - then.t[r];
    for (int r = 0; r < 3; ++r) for (int k = 0; k < 3; ++k) Du[r] += D[r * 3 + k] * u[k];
    for (int r = 0; r < 3; ++r) v[r] = Du[r] + now.t[r];
    for (int r = 0; r < 3; ++r) { shift[r] = b[r]; for (int k = 0; k < 3; ++k) shift[r] += B[r * 3 + k] * v[k]; }
    for (int r = 0; r < 3; ++r) { for (int c = 0; c < 3; ++c) out_rows[r * 4 + c] = (float)M[r * 3 + c]; out_rows[r * 4 + 3] = (float)shift[r]; }
    for (int r = 0; r < 3; ++r) { double s = a[r]; for (int k = 0; k < 3; ++k) s += A[r * 3 + k] * then.t[k]; then_view[r] = (float)s; }
    return true;
}

// a 3x4 (rows) applied to a point
inline void apply(const float rows[12], const float p[3], float out[3]) {
    for (int r = 0; r < 3; ++r) out[r] = rows[r * 4] * p[0] + rows[r * 4 + 1] * p[1] + rows[r * 4 + 2] * p[2] + rows[r * 4 + 3];
}
// how far the 3x4 turns (degrees)
inline double turn_deg(const float rows[12]) {
    const double tr = (double)rows[0] + rows[5] + rows[10];
    const double c = (tr - 1.0) * 0.5;
    return std::acos(c > 1.0 ? 1.0 : c < -1.0 ? -1.0 : c) * 57.29577951308232;
}

} // namespace sh2vr::carry_math
