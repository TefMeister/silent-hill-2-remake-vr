// fingers.cpp -- see fingers.h. Function names of hands_animation.lua / animation.lua are kept in the comments.
#include "fingers.h"

#include <map>
#include <vector>

#include "animation.h"
#include "bridge.h"
#include "params.h"
#include "settings.h"
#include "ue.h"

namespace sh2vr::fingers {

namespace {
using ue::Rot;

struct Handler {            // animations[animID] = {component, definitions}; inputHandlerAnimID[animID] = suffix
    std::string id;         // "left_arms"
    std::string target;     // "arms" (the part after the last "_")
    API::UObject* comp = nullptr;
};
struct Lerp {               // libs/core/lerp.lua, one per animID-animName
    std::string id, anim;
    double start = 0, end = 1, duration = 1, progress = 0, current = 0;
    bool running = false;
};
std::vector<Handler> g_handlers;
std::map<std::string, std::map<std::string, bool>> g_states;   // animStates[animID][animName]
std::map<std::string, Lerp> g_lerps;                           // lerpList[animID.."-"..animName]
std::string g_last_grip[2] = {"?", "?"};                       // what the bridge last said each hand holds
int g_last_montage = -1;

const char* hand_str(Hand h) { return h == LEFT ? "left" : "right"; }

// the pack's "animations": the first (only) definition in hands_parameters.json, as ik.lua picks it
const json::Value* defs() {
    const json::Value* f = params::file("hands_parameters.json");
    const json::Value* a = f != nullptr ? f->get("animations") : nullptr;
    if (a == nullptr || a->kind != json::Value::OBJ || a->obj.empty()) return nullptr;
    return &a->obj.begin()->second;
}
const json::Value* positions(const std::string& anim_name) {
    const json::Value* d = defs();
    const json::Value* p = d != nullptr ? d->get("positions") : nullptr;
    return p != nullptr ? p->get(anim_name) : nullptr;
}

API::UObject* component_of(const std::string& id) {
    for (const auto& h : g_handlers) if (h.id == id) return h.comp;
    return nullptr;
}

Rot rot_of(const json::Value& angles) {   // [pitch, yaw, roll]
    if (angles.kind != json::Value::ARR || angles.arr.size() < 3) return Rot{};
    return Rot{angles.arr[0].n, angles.arr[1].n, angles.arr[2].n};
}

// doAnimate: every bone of the position set to its rotator, in bone space
void do_animate(API::UObject* comp, const json::Value* anim) {
    if (anim == nullptr || anim->kind != json::Value::OBJ || !ue::alive(comp)) return;
    for (const auto& [bone, angles] : anim->obj) anim::set_local_rotator(comp, ue::name(std::wstring(bone.begin(), bone.end())), rot_of(angles));
}

void animate(const std::string& id, const std::string& anim_name, const char* val) {   // animate(animID, animName, "on"/"off")
    const json::Value* sub = positions(anim_name);
    if (sub != nullptr) do_animate(component_of(id), sub->get(val));
}

// lerpAnimation: each bone between its "off" and "on" rotator, KismetMathLibrary.RLerp with the shortest path
void lerp_animation(const std::string& id, const std::string& anim_name, double alpha) {
    API::UObject* comp = component_of(id);
    const json::Value* sub = positions(anim_name);
    if (sub == nullptr || !ue::alive(comp)) return;
    const json::Value* start = sub->get("off");
    const json::Value* end = sub->get("on");
    if (start == nullptr) start = end;
    if (end == nullptr) end = start;
    if (start == nullptr || start->kind != json::Value::OBJ || end->kind != json::Value::OBJ) return;
    auto* kml = ue::cdo(L"Class /Script/Engine.KismetMathLibrary");
    for (const auto& [bone, angles] : start->obj) {
        const json::Value* to = end->get(bone);
        if (to == nullptr) continue;
        Rot r;
        if (!ue::call_ret(kml, L"RLerp", {rot_of(angles), rot_of(*to), (float)alpha, true}, r)) return;
        anim::set_local_rotator(comp, ue::name(std::wstring(bone.begin(), bone.end())), r);
    }
}

void pose(const std::string& id, const std::string& pose_id) {   // pose(animID, poseID): [[animName, "on"/"off"], ...]
    const json::Value* d = defs();
    const json::Value* poses = d != nullptr ? d->get("poses") : nullptr;
    const json::Value* p = poses != nullptr ? poses->get(pose_id) : nullptr;
    if (p == nullptr || p->kind != json::Value::ARR) return;
    for (const auto& step : p->arr) {
        if (step.kind != json::Value::ARR || step.arr.size() < 2 || step.arr[0].kind != json::Value::STR || step.arr[1].kind != json::Value::STR) continue;
        animate(id, step.arr[0].s, step.arr[1].s.c_str());
    }
}

// uevrUtils.lerp: start a lerp, or steer a running one towards the new end keeping its current alpha
void start_lerp(const std::string& id, const std::string& anim_name, double from, double to, double duration) {
    const std::string key = id + "-" + anim_name;
    auto it = g_lerps.find(key);
    if (it == g_lerps.end()) {
        Lerp l;
        l.id = id; l.anim = anim_name; l.start = from; l.end = to; l.duration = duration; l.current = from; l.running = true;
        g_lerps[key] = l;
        return;
    }
    Lerp& l = it->second;   // update(): the remaining distance keeps the same pace
    const double span = to - from;
    l.duration = span != 0.0 ? (to - l.current) / span * duration : 0.0;
    l.progress = 0;
    l.start = l.current;
    l.end = to;
    l.running = true;
}

void update_animation(const std::string& id, const std::string& anim_name, bool pressed) {   // updateAnimation(..., {duration = 0.1})
    bool& state = g_states[id][anim_name];   // a new entry starts false, like the Lua
    if (pressed) {
        if (!state) start_lerp(id, anim_name, 0.0, 1.0, settings::FINGER_LERP_S);
        state = true;
    } else {
        if (state) start_lerp(id, anim_name, 1.0, 0.0, settings::FINGER_LERP_S);
        state = false;
    }
}

void reset_animation(const std::string& id, const std::string& anim_name, bool pressed) { g_states[id][anim_name] = pressed; }   // resetAnimation

std::vector<std::string> unique_targets() {   // forEachUniqueTarget
    std::vector<std::string> out;
    for (const auto& h : g_handlers) {
        bool seen = false;
        for (const auto& t : out) if (t == h.target) seen = true;
        if (!seen) out.push_back(h.target);
    }
    return out;
}

// getAttachmentStateAndExtension, from the bridge's word for the hand: "-" nothing held, "+" the default grip,
// else the grip animation's name ("pistol" -> "_pistol")
bool held(Hand hand, std::string& ext) {
    const std::string g = hand == LEFT ? bridge::grip_left() : bridge::grip_right();
    ext.clear();
    if (g.empty() || g == "-" || g == "attachment_none") return false;
    if (g != "+") ext = "_" + g;
    return true;
}

void update_state(Hand hand) {   // updateAnimationState
    if (g_handlers.empty()) return;
    std::string ext;
    const bool holding = held(hand, ext);
    const std::string hs = hand_str(hand);
    for (const auto& target : unique_targets()) {
        const std::string id = hs + "_" + target;
        if (holding) {
            reset_animation(id, hs + "_grip_weapon" + ext, true);
            reset_animation(id, hs + "_trigger_weapon" + ext, true);
            pose(id, "grip_" + hs + "_weapon" + ext);
        } else {
            pose(id, "open_" + hs);
        }
    }
}

bool thumb_resting(Hand hand) {   // is_action_active(ThumbrestTouch<side>, that side's joystick)
    static UEVR_ActionHandle left = API::VR::get_action_handle(settings::THUMBREST_LEFT);
    static UEVR_ActionHandle right = API::VR::get_action_handle(settings::THUMBREST_RIGHT);
    const UEVR_InputSourceHandle src = hand == RIGHT ? API::VR::get_right_joystick_source() : API::VR::get_left_joystick_source();
    return API::VR::is_action_active(hand == RIGHT ? right : left, src);
}
} // namespace

void add(const std::string& anim_id, API::UObject* component) {
    if (component == nullptr || anim_id.empty()) return;
    // the IK arms register both hands on ONE mesh (first run 2026-10-24: "finger poses on 1 hand(s)", the left was
    // dropped by a remove-by-component here), so only a handler with the same name is replaced
    for (size_t i = 0; i < g_handlers.size();) {
        if (g_handlers[i].id == anim_id) g_handlers.erase(g_handlers.begin() + (ptrdiff_t)i); else ++i;
    }
    Handler h;
    h.id = anim_id;
    const size_t us = anim_id.rfind('_');
    h.target = us == std::string::npos ? anim_id : anim_id.substr(us + 1);
    h.comp = component;
    g_handlers.push_back(h);
    g_states.erase(anim_id);
    const Hand hand = anim_id.rfind("left", 0) == 0 ? LEFT : RIGHT;
    update_state(hand);   // the Lua waited for the next grip change; starting in the right pose costs nothing
    LOGI("%s fingers: %s poses on %s (%d hand(s) registered)", TAG, anim_id.c_str(), ue::name_of(component).c_str(), (int)g_handlers.size());
}

void remove(API::UObject* component) {
    for (size_t i = 0; i < g_handlers.size();) {
        if (g_handlers[i].comp == component) {
            g_states.erase(g_handlers[i].id);
            for (auto it = g_lerps.begin(); it != g_lerps.end();) it = it->second.id == g_handlers[i].id ? g_lerps.erase(it) : std::next(it);
            g_handlers.erase(g_handlers.begin() + (ptrdiff_t)i);
        } else {
            ++i;
        }
    }
}

void on_input(const XINPUT_STATE& state) {   // handleInputForHands(state, rightAttachment, leftAttachment, nil, true)
    if (g_handlers.empty()) return;
    for (const auto& target : unique_targets()) {
        for (int hi = LEFT; hi <= RIGHT; ++hi) {
            const Hand hand = (Hand)hi;
            const std::string hs = hand_str(hand);
            const std::string id = hs + "_" + target;
            std::string ext;
            const bool holding = held(hand, ext);
            const int trigger = hand == RIGHT ? state.Gamepad.bRightTrigger : state.Gamepad.bLeftTrigger;
            const bool shoulder = (state.Gamepad.wButtons & (hand == RIGHT ? XINPUT_GAMEPAD_RIGHT_SHOULDER : XINPUT_GAMEPAD_LEFT_SHOULDER)) != 0;
            if (!holding) {
                update_animation(id, hs + "_trigger", trigger > settings::TRIGGER_PRESSED_OVER);
                update_animation(id, hs + "_grip", shoulder);
                update_animation(id, hs + "_thumb", thumb_resting(hand));
            } else {
                update_animation(id, hs + "_trigger_weapon" + ext, trigger > settings::TRIGGER_PRESSED_OVER);
                update_animation(id, hs + "_grip_weapon" + ext, shoulder);
            }
        }
    }
}

void tick(float delta) {
    if (g_handlers.empty()) return;
    // a hand's held weapon changed (the pack's attachments module decided it), or a montage ended: re-pose
    for (int hi = LEFT; hi <= RIGHT; ++hi) {
        const std::string g = hi == LEFT ? bridge::grip_left() : bridge::grip_right();
        if (g != g_last_grip[hi]) { g_last_grip[hi] = g; update_state((Hand)hi); }
    }
    if (bridge::montage_count() != g_last_montage) {
        g_last_montage = bridge::montage_count();
        update_state(LEFT);
        update_state(RIGHT);
    }
    // the lerps (uevr_utils updateLerp + lerpCallback)
    for (auto it = g_lerps.begin(); it != g_lerps.end();) {
        Lerp& l = it->second;
        if (!l.running) { it = g_lerps.erase(it); continue; }
        l.progress += delta;
        const double t = l.duration <= 0.0 ? 1.0 : std::min(l.progress / l.duration, 1.0);
        l.current = l.duration <= 0.0 ? l.end : l.start + (l.end - l.start) * t;
        lerp_animation(l.id, l.anim, l.current);
        if (t >= 1.0) l.running = false;
        ++it;
    }
}

bool exists() { return !g_handlers.empty(); }
int count() { return (int)g_handlers.size(); }

} // namespace sh2vr::fingers
