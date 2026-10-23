// ik.cpp -- see ik.h. A line-for-line port of jbusfield's ik.lua; its function names are kept in the comments so the
// two can be read side by side. Rotator maths goes through KismetMathLibrary exactly where the Lua's did; pure
// vector maths and the pack's own quaternion helpers are in ikmath.h.
//
// Not carried over (none of it is on in Tefa's settings): parent_type 2 (spring arm on the head) and 3, the debug
// spheres, accessories as IK targets (on_accessory_attach), the gunstock rotation, the config-menu live updates.
#include "ik.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "controllers.h"
#include "ikmath.h"
#include "params.h"
#include "settings.h"
#include "ue.h"

namespace sh2vr::ik {

using namespace ikmath;
using ue::Quat;
using ue::Rot;
using ue::Transform;
using ue::Vec3;

namespace {
constexpr int WS = 0, CS = 1;   // EBoneSpaces::WorldSpace, ComponentSpace

// ---- Kismet (the Lua's kismet_math_library calls) ----------------------------------------------------------------
API::UObject* kml() { return ue::cdo(L"Class /Script/Engine.KismetMathLibrary"); }
Rot compose_rotators(Rot a, Rot b) { Rot r; ue::call_ret(kml(), L"ComposeRotators", {a, b}, r); return r; }
Rot inverse_transform_rotation(const Transform& t, Rot r) { Rot o; ue::call_ret(kml(), L"InverseTransformRotation", {t, r}, o); return o; }
Vec3 inverse_transform_direction(const Transform& t, Vec3 v) { Vec3 o; ue::call_ret(kml(), L"InverseTransformDirection", {t, v}, o); return o; }
Vec3 transform_direction(const Transform& t, Vec3 v) { Vec3 o; ue::call_ret(kml(), L"TransformDirection", {t, v}, o); return o; }
Rot transform_rotation(const Transform& t, Rot r) { Rot o; ue::call_ret(kml(), L"TransformRotation", {t, r}, o); return o; }
Transform compose_transforms(const Transform& a, const Transform& b) { Transform o; ue::call_ret(kml(), L"ComposeTransforms", {a, b}, o); return o; }
Transform invert_transform(const Transform& t) { Transform o; ue::call_ret(kml(), L"InvertTransform", {t}, o); return o; }
Vec3 forward_of(Rot r) { Vec3 o; ue::call_ret(kml(), L"GetForwardVector", {r}, o); return o; }
Vec3 right_of(Rot r) { Vec3 o; ue::call_ret(kml(), L"GetRightVector", {r}, o); return o; }
Vec3 up_of(Rot r) { Vec3 o; ue::call_ret(kml(), L"GetUpVector", {r}, o); return o; }
Vec3 unrotate(Vec3 v, Rot r) { Vec3 o; ue::call_ret(kml(), L"LessLess_VectorRotator", {v, r}, o); return o; }   // vector << rotator
Rot make_rot_from_xz(Vec3 x, Vec3 z) { Rot o; ue::call_ret(kml(), L"MakeRotFromXZ", {x, z}, o); return o; }
Quat quat_from_euler_kismet(Rot r) { Quat q; ue::call_ret(kml(), L"Quat_MakeFromEuler", {Vec3{r.roll, r.pitch, r.yaw}}, q); return q; }   // Roll, Pitch, Yaw, as the Lua fed it
double fclamp(double v, double lo, double hi) { return v < lo ? lo : v > hi ? hi : v; }

// ---- the mesh (UPoseableMeshComponent) --------------------------------------------------------------------------
Vec3 bone_loc(API::UObject* m, const API::FName& b, int space) { Vec3 o; ue::call_ret(m, L"GetBoneLocationByName", {b, (int32_t)space}, o); return o; }
Rot bone_rot(API::UObject* m, const API::FName& b, int space) { Rot o; ue::call_ret(m, L"GetBoneRotationByName", {b, (int32_t)space}, o); return o; }
void set_bone_rot(API::UObject* m, const API::FName& b, Rot r, int space) { ue::call(m, L"SetBoneRotationByName", {b, r, (int32_t)space}); }
Transform bone_xform(API::UObject* m, const API::FName& b, int space) { Transform o; ue::call_ret(m, L"GetBoneTransformByName", {b, (int32_t)space}, o); return o; }
void set_bone_xform(API::UObject* m, const API::FName& b, const Transform& t, int space) { ue::call(m, L"SetBoneTransformByName", {b, t, (int32_t)space}); }
void set_bone_scale(API::UObject* m, const API::FName& b, Vec3 s, int space) { ue::call(m, L"SetBoneScaleByName", {b, s, (int32_t)space}); }
API::FName parent_bone(API::UObject* m, const API::FName& b) { API::FName o; ue::call_ret(m, L"GetParentBone", {b}, o); return o; }
API::FName bone_name(API::UObject* m, int32_t i) { API::FName o; ue::call_ret(m, L"GetBoneName", {i}, o); return o; }
int32_t num_bones(API::UObject* m) { int32_t n = 0; ue::call_ret(m, L"GetNumBones", {}, n); return n; }
Transform comp_to_world(API::UObject* m) { Transform o; ue::call_ret(m, L"K2_GetComponentToWorld", {}, o); return o; }
Vec3 comp_vec(API::UObject* m, const wchar_t* fn) { Vec3 o; ue::call_ret(m, fn, {}, o); return o; }   // GetRightVector / GetForwardVector / GetUpVector

// ---- settings (ik_parameters.json, the active profile) --------------------------------------------------------------
struct TwistBone { std::string bone; API::FName fname; double pitch = 0, yaw = 0, roll = 0; };
struct Solver {
    std::string id, label;
    std::string start_bone, joint_bone, end_bone, wrist_bone;
    API::FName start_f, joint_f, end_f, wrist_f;
    controllers::Side hand = controllers::RIGHT;
    int sort_order = 0;
    Vec3 hand_offset;            // end_bone_offset
    Rot end_bone_rotation;
    bool allow_wrist_affects_elbow = false, allow_stretch = false;
    double start_stretch_ratio = 0, max_stretch_scale = 0;
    double wrist_twist_influence = 0.35, wrist_twist_max = 75, forearm_twist_max = settings::FOREARM_TWIST_MAX_DEG;
    double smoothing = 0;
    bool lock_pitch = false, lock_yaw = false, lock_roll = false;
    std::vector<TwistBone> twist_bones;
    API::UObject* controller = nullptr;

    // newIKState + the per-bone align caches the Lua kept beside it
    struct Align { int order_swing = 0, order_twist = 0; };
    struct State {
        bool has_anchor = false; double anchor_right = 0, anchor_fwd = 0, anchor_up = 0;   // anchorPoleInBodySpace
        bool has_last_pole = false; Vec3 last_valid_pole_ws;
        bool has_sm_outward = false; Vec3 sm_outward_ws;
        bool has_ctrl_pole = false; Vec3 last_ctrl_pole_ws;
        bool lens_known = false; double upper_len = 0, lower_len = 0, upper_len_sq = 0, lower_len_sq = 0, max_reach = 0, outward_dist = 0;
        bool shoulder_axis_known = false; struct { char axis = 'Y'; int sign = 1; } shoulder_pole, joint_pole; bool joint_axis_known = false;
        Align shoulder, elbow;
        bool has_last_shoulder = false; Rot last_shoulder_rot;
        bool has_last_elbow = false; Rot last_elbow_rot;
        bool has_last_effector = false; Vec3 last_effector_cs;
        bool has_sm_dirs = false; Vec3 sm_upper_cs, sm_lower_cs, sm_pole_cs;
        bool has_last_ctrl_rot = false; Rot last_ctrl_rot_ws;
        bool default_rot_known = false; Rot default_rot;                       // meshDefaultRotation[1]
        std::map<std::string, std::pair<Vec3, Vec3>> twist_vecs;              // twistBoneVecs: x, z in lower-arm space
    } st;
};

struct Rig {
    API::UObject* mesh = nullptr;      // meshList[1]
    API::UObject* actor = nullptr;     // the actor of our own the mesh lives on
    API::UObject* pawn = nullptr;
    API::UObject* animation_mesh = nullptr;
    Vec3 mesh_loc_off; Rot mesh_rot_off;
    Vec3 anim_loc_off; Rot anim_rot_off;
    int coupling = 1;
    std::vector<Solver> solvers;       // in sort order
    std::vector<std::pair<API::FName, Transform>> initial;   // every bone's component-space transform at creation
    std::string root_bone;
    bool was_animating = false;
    bool hidden = false;
};
Rig* g_rig = nullptr;
float g_since_try = settings::HANDS_RETRY_S;
bool g_logged_wait = false;

Transform identity() { return Transform{}; }
void destroy_rig(Rig* rig) {
    if (rig == nullptr) return;
    if (ue::alive(rig->mesh)) ue::call(rig->mesh, L"K2_DestroyComponent", {rig->mesh});
    if (ue::alive(rig->actor)) ue::call(rig->actor, L"K2_DestroyActor", {});
    delete rig;
}

// ---- the Lua's shared flags, published by scripts/sh2vr_bridge.lua through UEVR's mod values --------------------
bool flag(const char* key) {
    char buf[8]{};
    const auto* vr = API::get()->param()->vr;
    if (vr == nullptr) return false;
    vr->get_mod_value(key, buf, sizeof buf);
    return buf[0] == '1';
}

Vec3 vec3_of(const json::Value* v, Vec3 d) { return (v && v->kind == json::Value::ARR && v->arr.size() >= 3) ? Vec3{v->arr[0].n, v->arr[1].n, v->arr[2].n} : d; }
Rot rot_of(const json::Value* v, Rot d) { return (v && v->kind == json::Value::ARR && v->arr.size() >= 3) ? Rot{v->arr[0].n, v->arr[1].n, v->arr[2].n} : d; }
double num_of(const json::Value* v, double d) { return (v && v->kind == json::Value::NUM) ? v->n : d; }
bool bool_of(const json::Value* v, bool d) { return (v && v->kind == json::Value::BOOL) ? v->b : d; }
std::string str_of(const json::Value* v, const std::string& d) { return (v && v->kind == json::Value::STR) ? v->s : d; }
std::wstring wide(const std::string& s) { return std::wstring(s.begin(), s.end()); }

const json::Value* profile() {   // paramManager:getActiveProfile(): _profileState.currentEditingProfile, else "default"
    const json::Value* f = params::file("ik_parameters.json");
    if (f == nullptr) return nullptr;
    std::string name = "default";
    if (const auto* ps = f->get("_profileState")) name = str_of(ps->get("currentEditingProfile"), name);
    return f->get(name);
}

// ---- helpers from ik.lua ---------------------------------------------------------------------------------------------
Vec3 bone_dir_cs(API::UObject* m, const API::FName& from, const API::FName& to) {   // getBoneDirCS
    return safe_normalize(sub(bone_loc(m, to, CS), bone_loc(m, from, CS)));
}
bool bend_pole_ref_cs(API::UObject* m, const API::FName& root, const API::FName& joint, const API::FName& end, Vec3& out) {   // getBendPoleRefCS
    const Vec3 s = bone_loc(m, root, CS), j = bone_loc(m, joint, CS), e = bone_loc(m, end, CS);
    const Vec3 reach = safe_normalize(sub(e, s));
    if (shorter_than(reach, 0.0001)) return false;
    const Vec3 pole = safe_normalize(project_on_plane(sub(j, s), reach));
    if (shorter_than(pole, 0.0001)) return false;
    out = pole;
    return true;
}
struct AxisChoice { char axis = 'X'; int sign = 1; double score = 0; };
AxisChoice choose_best_axis(Vec3 x, Vec3 y, Vec3 z, Vec3 dir) {   // chooseBestAxis
    const double dx = dot(x, dir), dy = dot(y, dir), dz = dot(z, dir);
    const double adx = std::fabs(dx), ady = std::fabs(dy), adz = std::fabs(dz);
    if (adx >= ady && adx >= adz) return {'X', dx >= 0 ? 1 : -1, dx};
    if (ady >= adx && ady >= adz) return {'Y', dy >= 0 ? 1 : -1, dy};
    return {'Z', dz >= 0 ? 1 : -1, dz};
}
AxisChoice choose_best_pole_axis(Vec3 x, Vec3 y, Vec3 z, char long_axis, Vec3 pole) {   // chooseBestPoleAxis
    AxisChoice best{'Y', 1, 0};
    auto try_axis = [&](char c, Vec3 v) {
        if (c == long_axis) return;
        const double d = dot(v, pole), ad = std::fabs(d);
        if (ad > best.score) best = {c, d >= 0 ? 1 : -1, ad};
    };
    try_axis('X', x); try_axis('Y', y); try_axis('Z', z);
    return best;
}
Vec3 axis_from_rotator(Rot r, char axis) { return axis == 'X' ? forward_of(r) : axis == 'Y' ? right_of(r) : up_of(r); }   // axisVectorFromRotator

Rot compose_swing(Solver::Align& a, Vec3 current_dir, Rot current_rot, Vec3 desired_dir, Rot delta) {   // composeSwingWithCachedOrder
    if (a.order_swing == 1) return compose_rotators(delta, current_rot);
    if (a.order_swing == 2) return compose_rotators(current_rot, delta);
    const Rot c1 = compose_rotators(delta, current_rot), c2 = compose_rotators(current_rot, delta);
    const Vec3 local_dir = safe_normalize(unrotate(current_dir, current_rot));
    auto score = [&](Rot r) { return dot(safe_normalize(rotate(local_dir, r)), desired_dir); };
    const bool second = score(c2) > score(c1);
    a.order_swing = second ? 2 : 1;
    return second ? c2 : c1;
}
Rot compose_twist(Solver::Align& a, Rot swing, Rot delta, Vec3 desired_dir, Vec3 desired_pole, char pole_axis, int pole_sign) {   // composeTwistWithCachedOrder
    if (a.order_twist == 1) return compose_rotators(delta, swing);
    if (a.order_twist == 2) return compose_rotators(swing, delta);
    const Rot t1 = compose_rotators(delta, swing), t2 = compose_rotators(swing, delta);
    auto score = [&](Rot r) {
        const Vec3 p = safe_normalize(project_on_plane(mul(axis_from_rotator(r, pole_axis), pole_sign), desired_dir));
        return dot(p, desired_pole);
    };
    const bool second = score(t2) > score(t1);
    a.order_twist = second ? 2 : 1;
    return second ? t2 : t1;
}

// alignBoneAxisToDirCS: turn `bone` so its bone->child direction meets desired_dir, then twist its pole axis towards pole
Rot align_bone(API::UObject* m, const API::FName& bone, const API::FName& child, Vec3 desired_dir_cs, const AxisChoice* pole_axis, Vec3 pole_cs, Solver::Align& a) {
    const Rot current_rot = bone_rot(m, bone, CS);
    const Vec3 current_dir = bone_dir_cs(m, bone, child);
    if (shorter_than(current_dir, 0.0001)) return current_rot;
    const Vec3 desired_dir = safe_normalize(desired_dir_cs);
    if (shorter_than(desired_dir, 0.0001)) return current_rot;
    const double d = fclamp(dot(current_dir, desired_dir), -1.0, 1.0);
    const double swing_deg = std::acos(d) * RAD2DEG;
    Vec3 swing_axis = cross(current_dir, desired_dir);
    if (shorter_than(swing_axis, 0.0001)) {
        Vec3 pole = safe_normalize(pole_cs);
        if (shorter_than(pole, 0.0001)) pole = Vec3{0, 1, 0};   // VEC_UNIT_Y
        swing_axis = cross(current_dir, pole);
    }
    swing_axis = safe_normalize(swing_axis);
    if (shorter_than(swing_axis, 0.0001)) return current_rot;
    const double effective = std::max(0.0, swing_deg - settings::IK_ALIGN_SWING_DEADBAND_DEG);
    if (effective < settings::IK_MIN_SWING_DEG) return current_rot;
    const Rot swing = compose_swing(a, current_dir, current_rot, desired_dir, rotator_from_axis_angle(swing_axis, effective));
    if (pole_axis == nullptr) return swing;

    const Vec3 raw_desired = project_on_plane(pole_cs, desired_dir);
    const double raw_desired_len = size(raw_desired);
    if (raw_desired_len < settings::IK_POLE_MIN_LEN) return swing;
    const Vec3 desired_pole = mul(raw_desired, 1.0 / raw_desired_len);
    const Vec3 raw_current = project_on_plane(mul(axis_from_rotator(swing, pole_axis->axis), pole_axis->sign), desired_dir);
    const double raw_current_len = size(raw_current);
    if (raw_current_len < settings::IK_POLE_MIN_LEN) return swing;
    const Vec3 current_pole = mul(raw_current, 1.0 / raw_current_len);
    const double twist_deg = signed_angle_deg(current_pole, desired_pole, desired_dir);
    if (std::fabs(twist_deg) < settings::IK_MIN_TWIST_DEG) return swing;
    return compose_twist(a, swing, rotator_from_axis_angle(desired_dir, twist_deg), desired_dir, desired_pole, pole_axis->axis, pole_axis->sign);
}

// calculateJointTargetForwardDistance + getJointTarget
void measure_arm(Solver& s, Vec3 shoulder, Vec3 joint, Vec3 end) {
    auto& st = s.st;
    st.upper_len = size(sub(joint, shoulder));
    st.lower_len = size(sub(end, joint));
    st.outward_dist = st.upper_len * settings::IK_OUTWARD_FRACTION;
    st.upper_len_sq = st.upper_len * st.upper_len;
    st.lower_len_sq = st.lower_len * st.lower_len;
    st.max_reach = std::max((st.upper_len + st.lower_len) - 0.001, 0.001);
    st.lens_known = true;
}
Vec3 joint_target(Solver& s, Vec3 shoulder, Vec3 joint, Vec3 end, Vec3 reach_dir, double reach_len, Vec3 outward) {
    auto& st = s.st;
    if (!st.lens_known) measure_arm(s, shoulder, joint, end);
    if (reach_len < 0.0001) return joint;
    const double clamped = std::min(reach_len, st.max_reach);
    double along = (st.upper_len_sq - st.lower_len_sq + (clamped * clamped)) / (2.0 * clamped);
    if (along < 0.0) along = 0.0; else if (along > st.upper_len) along = st.upper_len;
    return add(shoulder, add(mul(reach_dir, along), mul(outward, st.outward_dist)));
}

// getMeshOutward: which way the elbow points
Vec3 mesh_outward(Rig& rig, Solver& s, Vec3 shoulder_ws, Vec3 joint_ws, Vec3 reach_ws, const Transform& c2w, Rot ctrl_rot_cs) {
    auto& st = s.st;
    API::UObject* m = rig.mesh;
    const Vec3 mesh_right = comp_vec(m, L"GetRightVector"), mesh_fwd = comp_vec(m, L"GetForwardVector"), mesh_up = comp_vec(m, L"GetUpVector");
    if (!st.has_anchor) {
        const Vec3 raw = safe_normalize(sub(joint_ws, shoulder_ws));
        st.anchor_right = dot(raw, mesh_right); st.anchor_fwd = dot(raw, mesh_fwd); st.anchor_up = dot(raw, mesh_up);
        st.has_anchor = true;
    }
    const Vec3 fallback = s.hand == controllers::LEFT ? mul(mesh_right, -1.0) : mesh_right;
    Vec3 outward = st.has_last_pole ? st.last_valid_pole_ws : fallback;
    const Vec3 anchor = add(add(mul(mesh_right, st.anchor_right), mul(mesh_fwd, st.anchor_fwd)), mul(mesh_up, st.anchor_up));
    const Vec3 perp = cross(reach_ws, cross(anchor, reach_ws));
    const double perp_len = size(perp);
    if (perp_len >= 0.0001) {
        Vec3 candidate = mul(perp, 1.0 / perp_len);
        if (dot(candidate, anchor) < 0.0) candidate = mul(candidate, -1.0);
        outward = candidate;
        st.last_valid_pole_ws = candidate; st.has_last_pole = true;
    }
    if (s.allow_wrist_affects_elbow && s.wrist_twist_influence > 0) {
        const Vec3 ctrl_up_ws = safe_normalize(transform_direction(c2w, up_of(ctrl_rot_cs)));
        const Vec3 up_proj = safe_normalize(project_on_plane(ctrl_up_ws, reach_ws));
        if (size(up_proj) > settings::IK_CTRL_POLE_MIN_LEN) { st.last_ctrl_pole_ws = up_proj; st.has_ctrl_pole = true; }
        if (st.has_ctrl_pole) {
            double raw = signed_angle_deg(outward, st.last_ctrl_pole_ws, reach_ws);
            raw = std::fmod(360.0 + raw, 360.0) - 180.0;   // Lua: (((360 + raw) % 360) - 180), raw already within +-180 so fmod matches
            raw = fclamp(raw, -s.wrist_twist_max, s.wrist_twist_max);
            const double applied = raw * s.wrist_twist_influence;
            if (std::fabs(applied) > 0.01) outward = safe_normalize(rotate(outward, rotator_from_axis_angle(reach_ws, applied)));
        }
    }
    if (st.has_sm_outward) {
        const double alpha = perp_len < settings::IK_POLE_SMOOTH_NEAR_LEN ? settings::IK_POLE_SMOOTH_ALPHA_NEAR : settings::IK_POLE_SMOOTH_ALPHA_BASE;
        const Vec3 mixed = add(mul(st.sm_outward_ws, alpha), mul(outward, 1.0 - alpha));
        const double mixed_len = size(mixed);
        if (mixed_len > 0.0001) {
            outward = mul(mixed, 1.0 / mixed_len);
            if (dot(outward, anchor) < 0.0) outward = mul(outward, -1.0);
            const Vec3 perp_sm = cross(reach_ws, cross(outward, reach_ws));
            const double perp_sm_len = size(perp_sm);
            if (perp_sm_len > 0.0001) {
                outward = mul(perp_sm, 1.0 / perp_sm_len);
                if (dot(outward, anchor) < 0.0) outward = mul(outward, -1.0);
            }
        }
    }
    st.sm_outward_ws = outward; st.has_sm_outward = true;
    st.last_valid_pole_ws = outward; st.has_last_pole = true;
    return outward;
}

// getShoulderPoleAxis / getJointPoleAxis: which local axis of the bone should face the elbow's outward side (once)
AxisChoice pole_axis_for(API::UObject* m, const API::FName& bone, const API::FName& child, const API::FName& root, const API::FName& joint, const API::FName& end, controllers::Side hand) {
    const Vec3 dir = bone_dir_cs(m, bone, child);
    const Rot r = bone_rot(m, bone, CS);
    const Vec3 x = safe_normalize(forward_of(r)), y = safe_normalize(right_of(r)), z = safe_normalize(up_of(r));
    const AxisChoice longest = choose_best_axis(x, y, z, dir);
    Vec3 pole_ref;
    if (!bend_pole_ref_cs(m, root, joint, end, pole_ref)) pole_ref = hand == controllers::LEFT ? Vec3{0, -1, 0} : Vec3{0, 1, 0};
    return choose_best_pole_axis(x, y, z, longest.axis, pole_ref);
}

Rot smooth_rot(Rot prev, Rot cur, double a) {   // the Lua's per-axis blend with normalizeDeg180
    return Rot{prev.pitch + normalize_deg180(cur.pitch - prev.pitch) * a, prev.yaw + normalize_deg180(cur.yaw - prev.yaw) * a, prev.roll + normalize_deg180(cur.roll - prev.roll) * a};
}

// setBoneRotationByName (ik.lua's local): the hand bone, with the optional per-axis locks to its rest rotation
void set_end_bone(API::UObject* m, Solver& s, Rot final_rot) {
    auto& st = s.st;
    if (!st.default_rot_known) {   // cacheMeshBoneRotationState
        const API::FName parent = parent_bone(m, s.end_f);
        Rot rel = bone_rot(m, s.end_f, CS);
        if (!ue::is_none(parent)) {
            const Transform local = compose_transforms(bone_xform(m, s.end_f, CS), invert_transform(bone_xform(m, parent, CS)));
            rel = transform_rotation(local, Rot{});
        }
        st.default_rot = rel;
        st.default_rot_known = true;
    }
    if (s.lock_pitch || s.lock_yaw || s.lock_roll) {
        const Rot initial = transform_rotation(bone_xform(m, parent_bone(m, s.end_f), CS), st.default_rot);
        if (s.lock_pitch) final_rot.pitch = initial.pitch;
        if (s.lock_yaw) final_rot.yaw = initial.yaw;
        if (s.lock_roll) final_rot.roll = initial.roll;
    }
    set_bone_rot(m, s.end_f, final_rot, CS);
}

// initializeSolverState: each twist bone's x / z axes in lower-arm space, captured once
void init_twist_vecs(Rig& rig, Solver& s) {
    const Rot lower = bone_rot(rig.mesh, s.joint_f, CS);
    for (auto& tb : s.twist_bones) {
        if (s.st.twist_vecs.count(tb.bone)) continue;
        const Rot bone_cs = bone_rot(rig.mesh, tb.fname, CS);
        s.st.twist_vecs[tb.bone] = {unrotate(forward_of(bone_cs), lower), unrotate(up_of(bone_cs), lower)};
    }
}

// twistForearm: spread the wrist's twist along the forearm's twist bones
void twist_forearm(Rig& rig, Solver& s, Vec3 lower_dir_cs, Rot lower_rot_cs, Rot hand_rot_cs) {
    if (s.twist_bones.empty()) return;
    Quat swing{}; bool have_swing = false;
    double twist_deg = normalize_deg180(swing_twist(quat_from_euler_kismet(lower_rot_cs), quat_from_euler_kismet(hand_rot_cs), lower_dir_cs, swing, have_swing));
    twist_deg = fclamp(twist_deg, -s.forearm_twist_max, s.forearm_twist_max);
    const double half_deg_to_rad = PI / 360.0;
    for (const auto& tb : s.twist_bones) {
        auto it = s.st.twist_vecs.find(tb.bone);
        if (it == s.st.twist_vecs.end()) break;   // the Lua breaks the loop too
        Vec3 x = rotate(it->second.first, lower_rot_cs), z = rotate(it->second.second, lower_rot_cs);
        const double half = (twist_deg * tb.roll) * half_deg_to_rad;
        const double sh = std::sin(half);
        const Quat tube{lower_dir_cs.x * sh, lower_dir_cs.y * sh, lower_dir_cs.z * sh, std::cos(half)};
        x = rotate_quat(x, tube); z = rotate_quat(z, tube);
        if ((tb.pitch != 0 || tb.yaw != 0) && have_swing) {
            const Vec3 sx = rotate_quat(x, swing), sz = rotate_quat(z, swing);
            const Vec3 bx = safe_normalize(add(mul(x, 1.0 - tb.pitch), mul(sx, tb.pitch)));
            const Vec3 bz = safe_normalize(add(mul(z, 1.0 - tb.yaw), mul(sz, tb.yaw)));
            if (!shorter_than(bx, 0.0001)) x = bx;
            if (!shorter_than(bz, 0.0001)) z = bz;
        }
        set_bone_rot(rig.mesh, tb.fname, make_rot_from_xz(x, z), CS);
    }
}

// solveTwoBone
void solve_two_bone(Rig& rig, Solver& s) {
    auto& st = s.st;
    API::UObject* m = rig.mesh;
    if (!ue::alive(s.controller)) { s.controller = controllers::find(s.hand); if (s.controller == nullptr) return; }
    Vec3 ctrl_pos_ws; Rot ctrl_rot_ws;   // getTargetLocationAndRotation (no accessory, no gunstock)
    if (!ue::call_ret(s.controller, L"K2_GetComponentLocation", {}, ctrl_pos_ws) || !ue::call_ret(s.controller, L"K2_GetComponentRotation", {}, ctrl_rot_ws)) return;

    const Transform c2w = comp_to_world(m);
    const Vec3 shoulder_ws = bone_loc(m, s.start_f, WS);
    Vec3 effector_ws = add(ctrl_pos_ws, rotate(s.hand_offset, ctrl_rot_ws));
    const Rot ctrl_rot_cs = inverse_transform_rotation(c2w, ctrl_rot_ws);
    if (s.smoothing > 0) {
        const Vec3 off_cs = inverse_transform_direction(c2w, sub(effector_ws, shoulder_ws));
        const Vec3 sm = st.has_last_effector ? add(mul(st.last_effector_cs, s.smoothing), mul(off_cs, 1 - s.smoothing)) : off_cs;
        st.last_effector_cs = sm; st.has_last_effector = true;
        effector_ws = add(shoulder_ws, transform_direction(c2w, sm));
    }
    const Vec3 shoulder_to_hand = sub(effector_ws, shoulder_ws);
    const double reach_len = size(shoulder_to_hand);
    if (reach_len < 0.0001) return;
    const Vec3 reach_dir = mul(shoulder_to_hand, 1.0 / reach_len);
    const Vec3 joint_ws = bone_loc(m, s.joint_f, WS), end_ws = bone_loc(m, s.end_f, WS);
    const Vec3 outward_ws = mesh_outward(rig, s, shoulder_ws, joint_ws, reach_dir, c2w, ctrl_rot_cs);
    const Vec3 target_ws = joint_target(s, shoulder_ws, joint_ws, end_ws, reach_dir, reach_len, outward_ws);

    Vec3 out_joint, out_end;   // K2_TwoBoneIK on KismetAnimationLibrary, the two out-parameters read back
    auto* kal = ue::cdo(L"Class /Script/AnimGraphRuntime.KismetAnimationLibrary");
    if (!ue::call(kal, L"K2_TwoBoneIK", {shoulder_ws, joint_ws, end_ws, target_ws, effector_ws, &out_joint, &out_end, s.allow_stretch, (float)s.start_stretch_ratio, (float)s.max_stretch_scale})) return;

    Vec3 upper_cs = safe_normalize(inverse_transform_direction(c2w, safe_normalize(sub(out_joint, shoulder_ws))));
    Vec3 lower_cs = safe_normalize(inverse_transform_direction(c2w, safe_normalize(sub(out_end, out_joint))));
    Vec3 pole_cs = safe_normalize(inverse_transform_direction(c2w, outward_ws));
    if (s.smoothing > 0) {
        auto nlerp = [](bool have, Vec3 prev, Vec3 cur, double alpha) {
            if (!have) return cur;
            const Vec3 mixed = add(mul(prev, alpha), mul(cur, 1.0 - alpha));
            const double len = size(mixed);
            return len < 0.0001 ? cur : mul(mixed, 1.0 / len);
        };
        upper_cs = nlerp(st.has_sm_dirs, st.sm_upper_cs, upper_cs, settings::IK_DIR_SMOOTH_ALPHA);
        lower_cs = nlerp(st.has_sm_dirs, st.sm_lower_cs, lower_cs, settings::IK_DIR_SMOOTH_ALPHA);
        pole_cs = nlerp(st.has_sm_dirs, st.sm_pole_cs, pole_cs, settings::IK_POLE_DIR_SMOOTH_ALPHA);
        st.sm_upper_cs = upper_cs; st.sm_lower_cs = lower_cs; st.sm_pole_cs = pole_cs; st.has_sm_dirs = true;
    }
    if (!st.shoulder_axis_known) { const auto c = pole_axis_for(m, s.start_f, s.joint_f, s.start_f, s.joint_f, s.end_f, s.hand); st.shoulder_pole.axis = c.axis; st.shoulder_pole.sign = c.sign; st.shoulder_axis_known = true; }
    if (!st.joint_axis_known) { const auto c = pole_axis_for(m, s.joint_f, s.end_f, s.start_f, s.joint_f, s.end_f, s.hand); st.joint_pole.axis = c.axis; st.joint_pole.sign = c.sign; st.joint_axis_known = true; }
    const AxisChoice shoulder_axis{st.shoulder_pole.axis, st.shoulder_pole.sign, 0}, joint_axis{st.joint_pole.axis, st.joint_pole.sign, 0};

    // rotateShoulder
    if (s.smoothing > 0 && st.has_last_shoulder) set_bone_rot(m, s.start_f, st.last_shoulder_rot, CS);
    Rot shoulder_rot = align_bone(m, s.start_f, s.joint_f, upper_cs, &shoulder_axis, pole_cs, st.shoulder);
    if (s.smoothing > 0) {
        if (st.has_last_shoulder) shoulder_rot = smooth_rot(st.last_shoulder_rot, shoulder_rot, 1 - s.smoothing);
        st.last_shoulder_rot = shoulder_rot; st.has_last_shoulder = true;
    }
    set_bone_rot(m, s.start_f, shoulder_rot, CS);

    // rotateElbow
    if (s.smoothing > 0 && st.has_last_elbow) set_bone_rot(m, s.joint_f, st.last_elbow_rot, CS);
    Rot elbow_rot = align_bone(m, s.joint_f, s.end_f, lower_cs, &joint_axis, pole_cs, st.elbow);
    if (s.smoothing > 0 && st.has_last_elbow) elbow_rot = smooth_rot(st.last_elbow_rot, elbow_rot, 1 - s.smoothing);
    set_bone_rot(m, s.joint_f, elbow_rot, CS);
    st.last_elbow_rot = elbow_rot; st.has_last_elbow = true;

    // rotateHandAndWrist
    Rot hand_rot_cs;
    if (s.smoothing > 0) {
        Rot sm_ws = st.has_last_ctrl_rot ? smooth_rot(st.last_ctrl_rot_ws, ctrl_rot_ws, 1.0 - s.smoothing) : ctrl_rot_ws;
        st.last_ctrl_rot_ws = sm_ws; st.has_last_ctrl_rot = true;
        hand_rot_cs = compose_rotators(s.end_bone_rotation, inverse_transform_rotation(c2w, sm_ws));
        set_end_bone(m, s, hand_rot_cs);
        if (!s.wrist_bone.empty()) set_bone_rot(m, s.wrist_f, compose_rotators(s.end_bone_rotation, sm_ws), WS);
    } else {
        hand_rot_cs = compose_rotators(s.end_bone_rotation, ctrl_rot_cs);
        set_end_bone(m, s, hand_rot_cs);
        if (!s.wrist_bone.empty()) set_bone_rot(m, s.wrist_f, hand_rot_cs, CS);
    }
    twist_forearm(rig, s, lower_cs, elbow_rot, hand_rot_cs);
}

// ---- building the rig (Rig:create, initializeRigState, updateBonesVisibility, setActive) -------------------------
bool load_solver(Rig& rig, const std::string& id, const json::Value& sp, Solver& s) {
    if (!bool_of(sp.get("active"), false)) return false;
    s.id = id;
    s.label = str_of(sp.get("label"), id);
    s.end_bone = str_of(sp.get("end_bone"), "");
    if (s.end_bone.empty()) return false;
    s.end_f = ue::name(wide(s.end_bone));
    // getAncestorBones(mesh, end_bone, 3) fills start / joint when the file leaves them out
    s.joint_bone = str_of(sp.get("joint_bone"), "");
    s.start_bone = str_of(sp.get("start_bone"), "");
    if (s.joint_bone.empty() || s.start_bone.empty()) {
        const API::FName p1 = parent_bone(rig.mesh, s.end_f), p2 = parent_bone(rig.mesh, p1), p3 = parent_bone(rig.mesh, p2);
        if (ue::is_none(p3)) { LOGW("%s ik: solver %s: fewer than 3 bones above %s", TAG, s.label.c_str(), s.end_bone.c_str()); return false; }
        if (s.joint_bone.empty()) s.joint_bone = narrow(ue::str(p2));
        if (s.start_bone.empty()) s.start_bone = narrow(ue::str(p3));
    }
    s.joint_f = ue::name(wide(s.joint_bone));
    s.start_f = ue::name(wide(s.start_bone));
    s.wrist_bone = str_of(sp.get("wrist_bone"), "");
    if (!s.wrist_bone.empty()) s.wrist_f = ue::name(wide(s.wrist_bone));
    s.hand = (int)num_of(sp.get("end_control_type"), 1) == 0 ? controllers::LEFT : controllers::RIGHT;
    s.sort_order = (int)num_of(sp.get("sort_order"), 0);
    s.hand_offset = vec3_of(sp.get("end_bone_offset"), Vec3{});
    s.end_bone_rotation = rot_of(sp.get("end_bone_rotation"), Rot{});
    s.allow_wrist_affects_elbow = bool_of(sp.get("allow_wrist_affects_elbow"), false);
    s.allow_stretch = bool_of(sp.get("allow_stretch"), false);
    s.start_stretch_ratio = num_of(sp.get("start_stretch_ratio"), 0.0);
    s.max_stretch_scale = num_of(sp.get("max_stretch_scale"), 0.0);
    s.wrist_twist_influence = num_of(sp.get("wrist_twist_influence"), 0.35);
    s.wrist_twist_max = num_of(sp.get("wrist_twist_max"), 75.0);
    s.forearm_twist_max = num_of(sp.get("forearm_twist_max"), settings::FOREARM_TWIST_MAX_DEG);
    s.smoothing = num_of(sp.get("smoothing"), 0.0);
    s.lock_pitch = bool_of(sp.get("end_bone_lock_pitch"), false);
    s.lock_yaw = bool_of(sp.get("end_bone_lock_yaw"), false);
    s.lock_roll = bool_of(sp.get("end_bone_lock_roll"), false);
    if (const auto* tb = sp.get("twist_bones"); tb != nullptr && tb->kind == json::Value::ARR) {
        for (const auto& e : tb->arr) {
            TwistBone t;
            t.bone = str_of(e.get("bone"), "");
            if (t.bone.empty()) continue;
            t.fname = ue::name(wide(t.bone));
            t.pitch = num_of(e.get("pitch"), 0.0); t.yaw = num_of(e.get("yaw"), 0.0);
            t.roll = num_of(e.get("fraction"), num_of(e.get("roll"), 0.0));   // the Lua read fraction, else roll
            s.twist_bones.push_back(t);
        }
    }
    s.controller = controllers::find(s.hand);
    return true;
}

void collapse_body(Rig& rig, const json::Value& prof) {   // updateBonesVisibility with hide_body
    if (!bool_of(prof.get("hide_body"), false)) return;
    const std::string spine = str_of(prof.get("spine_bone_name"), ""), ls = str_of(prof.get("left_shoulder_bone_name"), ""), rs = str_of(prof.get("right_shoulder_bone_name"), "");
    if (spine.empty() || ls.empty() || rs.empty()) return;
    const double w = num_of(prof.get("shoulder_width_scale"), 1.0);
    const API::FName root = bone_name(rig.mesh, 0);
    const Vec3 one{1, 1, 1}, tiny{settings::HIDDEN_BONE_SCALE, settings::HIDDEN_BONE_SCALE, settings::HIDDEN_BONE_SCALE};
    set_bone_scale(rig.mesh, root, one, CS);
    set_bone_scale(rig.mesh, root, tiny, CS);
    set_bone_scale(rig.mesh, ue::name(wide(spine)), Vec3{settings::HIDDEN_BONE_SCALE, settings::HIDDEN_BONE_SCALE, w}, CS);
    set_bone_scale(rig.mesh, ue::name(wide(ls)), one, CS);
    set_bone_scale(rig.mesh, ue::name(wide(rs)), one, CS);
}

bool create(API::UObject* pawn) {
    const json::Value* prof = profile();
    if (prof == nullptr) { if (!g_logged_wait) LOGW("%s ik: no profile in ik_parameters.json", TAG); g_logged_wait = true; return false; }
    const std::string mesh_name = str_of(prof->get("mesh"), "");
    if (mesh_name.empty()) return false;   // the Lua's tryAutoCreateArms needs a mesh name too
    if (ue::cdo(L"Class /Script/AnimGraphRuntime.KismetAnimationLibrary") == nullptr) { if (!g_logged_wait) LOGW("%s ik: KismetAnimationLibrary not found, IK off", TAG); g_logged_wait = true; return false; }
    API::UObject* root = nullptr;
    if (auto** r = ue::prop<API::UObject*>(pawn, L"RootComponent")) root = *r;
    API::UObject* source = nullptr;   // "Pawn.Mesh"
    if (mesh_name == "Pawn.Mesh") { if (auto** m = ue::prop<API::UObject*>(pawn, L"Mesh")) source = *m; }
    else { if (!g_logged_wait) LOGW("%s ik: mesh descriptor %s not supported yet (only Pawn.Mesh)", TAG, mesh_name.c_str()); g_logged_wait = true; return false; }
    if (root == nullptr || source == nullptr || controllers::find(controllers::LEFT) == nullptr || controllers::find(controllers::RIGHT) == nullptr) {
        if (!g_logged_wait) LOGI("%s ik: waiting for the pawn's root, its mesh and both controllers", TAG);
        g_logged_wait = true;
        return false;
    }

    auto* rig = new Rig();
    rig->pawn = pawn;
    rig->coupling = (int)num_of(prof->get("parent_type"), 1);
    if (rig->coupling != 1) LOGW("%s ik: parent_type %d not ported, treating it as 1 (pawn root)", TAG, rig->coupling);
    rig->mesh_loc_off = vec3_of(prof->get("mesh_location_offset"), Vec3{});
    rig->mesh_rot_off = rot_of(prof->get("mesh_rotation_offset"), Rot{});
    rig->anim_loc_off = vec3_of(prof->get("animation_location_offset"), Vec3{});
    rig->anim_rot_off = rot_of(prof->get("animation_rotation_offset"), Rot{});

    // createPoseableMeshFromSkeletalMesh(source, useDefaultPose) on an actor of our own, attached to the pawn's root
    API::UObject* actor = nullptr;
    {
        auto* statics = ue::cdo(L"Class /Script/Engine.GameplayStatics");
        auto* actor_class = ue::find_class(L"Class /Script/Engine.Actor");
        const Transform t = identity();
        if (statics == nullptr || actor_class == nullptr || !ue::call_ret(statics, L"BeginDeferredActorSpawnFromClass", {pawn, (API::UObject*)actor_class, t, (int32_t)1, (API::UObject*)nullptr}, actor) || actor == nullptr) { delete rig; return false; }
        API::UObject* finished = nullptr;
        ue::call_ret(statics, L"FinishSpawningActor", {actor, t}, finished);
    }
    rig->actor = actor;
    API::UObject* mesh = nullptr;
    {
        auto* cls = ue::find_class(L"Class /Script/Engine.PoseableMeshComponent");
        if (cls == nullptr || !ue::call_ret(actor, L"AddComponentByClass", {(API::UObject*)cls, true, identity(), false}, mesh) || mesh == nullptr) { LOGW("%s ik: could not make the poseable mesh", TAG); destroy_rig(rig); return false; }
        ue::call(mesh, L"SetCollisionEnabled", {(int32_t)0});
        if (auto** dst = ue::prop<API::UObject*>(mesh, L"SkeletalMesh")) if (auto** src = ue::prop<API::UObject*>(source, L"SkeletalMesh")) *dst = *src;
        const wchar_t* follow = L"SetLeaderPoseComponent";
        if (!ue::call(mesh, follow, {source, true})) { follow = L"SetMasterPoseComponent"; ue::call(mesh, follow, {source, true}); }
        ue::call(mesh, follow, {(API::UObject*)nullptr, false});
        int32_t n = 0;
        ue::call_ret(source, L"GetNumMaterials", {}, n);
        for (int32_t i = 0; i < n; ++i) { API::UObject* mat = nullptr; if (ue::call_ret(source, L"GetMaterial", {i}, mat) && mat != nullptr) ue::call(mesh, L"SetMaterial", {i, mat}); }
    }
    rig->mesh = mesh;
    bool attached = false;
    ue::call_ret(mesh, L"K2_AttachTo", {root, ue::name(L""), (int32_t)0, false}, attached);
    ue::call(mesh, L"SetVisibility", {true, true});
    ue::call(mesh, L"SetHiddenInGame", {false, true});
    if (auto* b = ue::prop<float>(mesh, L"BoundsScale")) *b = (float)settings::HAND_BOUNDS_SCALE;
    if (auto* p = ue::prop<Vec3>(mesh, L"RelativeLocation")) *p = rig->mesh_loc_off;
    if (auto* p = ue::prop<Rot>(mesh, L"RelativeRotation")) *p = rig->mesh_rot_off;
    collapse_body(*rig, *prof);

    const std::string anim_name = str_of(prof->get("animation_mesh"), "");
    if (anim_name == "Pawn.Mesh") rig->animation_mesh = source;

    // setActive for every active solver, in sort order
    if (const auto* solvers = prof->get("solvers"); solvers != nullptr && solvers->kind == json::Value::OBJ) {
        for (const auto& [id, sp] : solvers->obj) {
            Solver s;
            if (load_solver(*rig, id, sp, s)) rig->solvers.push_back(std::move(s));
        }
    }
    std::sort(rig->solvers.begin(), rig->solvers.end(), [](const Solver& a, const Solver& b) { return a.sort_order == b.sort_order ? a.id < b.id : a.sort_order < b.sort_order; });
    for (auto& s : rig->solvers) init_twist_vecs(*rig, s);
    const int32_t n = num_bones(mesh);
    for (int32_t i = 0; i < n; ++i) { const API::FName b = bone_name(mesh, i); rig->initial.push_back({b, bone_xform(mesh, b, CS)}); }
    rig->root_bone = narrow(ue::str(bone_name(mesh, 0)));
    g_rig = rig;
    g_logged_wait = false;
    std::string names;
    for (const auto& s : rig->solvers) names += (names.empty() ? "" : ", ") + s.label + " (" + s.start_bone + " > " + s.joint_bone + " > " + s.end_bone + ", " + std::to_string(s.twist_bones.size()) + " twist bones)";
    LOGI("%s ik: arms made from %s, attached to the pawn root %s, %d bones, solvers: %s", TAG, mesh_name.c_str(), attached ? "yes" : "NO", n, names.c_str());
    return true;
}
} // namespace

void tick(float delta) {
    API::UObject* pawn = API::get()->get_local_pawn(0);
    if (g_rig != nullptr && (pawn != g_rig->pawn || !ue::alive(g_rig->mesh))) { LOGI("%s ik: the pawn or the arms went away, rebuilding", TAG); destroy(); }
    if (g_rig == nullptr) {
        if (pawn == nullptr) return;
        g_since_try += delta;
        if (g_since_try < settings::HANDS_RETRY_S) return;
        g_since_try = 0.0f;
        create(pawn);
        return;
    }
    Rig& rig = *g_rig;

    // the Lua's 200 ms hide check: the UI / montage modules answer is_hands_hidden; the bridge script publishes it
    const bool hidden = flag("sh2vr_hands_hidden");
    if (hidden != rig.hidden) { rig.hidden = hidden; ue::call(rig.mesh, L"SetVisibility", {!hidden, true}); }

    // tickFn, coupling 1: the mesh rides at the pawn's capsule height
    if (auto** r = ue::prop<API::UObject*>(rig.pawn, L"RootComponent"); r != nullptr && *r != nullptr) {
        float half = 0.0f;
        if (auto* h = ue::prop<float>(*r, L"CapsuleHalfHeight")) half = *h;
        if (auto* p = ue::prop<Vec3>(rig.mesh, L"RelativeLocation")) p->z = rig.mesh_loc_off.z + (rig.mesh_loc_off.z + half);
    }
    const bool animating = flag("sh2vr_anim_left") || flag("sh2vr_anim_right");
    bool did_animate = false;
    if (animating && ue::alive(rig.animation_mesh)) {   // animateFromMesh
        did_animate = ue::call(rig.mesh, L"CopyPoseFromSkeletalComponent", {rig.animation_mesh});
        if (did_animate) rig.was_animating = true;
    }
    if (did_animate) return;
    if (rig.was_animating) {   // setInitialTransform
        for (const auto& [b, t] : rig.initial) set_bone_xform(rig.mesh, b, t, CS);
        rig.was_animating = false;
    }
    for (auto& s : rig.solvers) solve_two_bone(rig, s);
}

bool exists() { return g_rig != nullptr; }
API::UObject* mesh() { return g_rig != nullptr ? g_rig->mesh : nullptr; }
void destroy() { destroy_rig(g_rig); g_rig = nullptr; }

} // namespace sh2vr::ik
