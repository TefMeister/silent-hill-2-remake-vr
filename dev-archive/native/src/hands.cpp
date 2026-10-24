// hands.cpp -- see hands.h. Function names of jbusfield's hands.lua / uevr_utils.lua kept in the comments.
#include "hands.h"

#include "animation.h"
#include "fingers.h"
#include "params.h"
#include "settings.h"
#include "ue.h"

namespace sh2vr::hands {

namespace {
Hand g_hands[2] = {Hand{LEFT}, Hand{RIGHT}};
API::UObject* g_actor = nullptr;   // the actor our hand components live on (uevr_utils.spawn_actor)
API::UObject* g_pawn = nullptr;    // the pawn the hands were made for
float g_since_try = settings::HANDS_RETRY_S;
bool g_logged_wait = false;

using ue::Rot;
using ue::Transform;
using ue::Vec3;

// uevr_utils.spawn_actor: an empty Actor in the pawn's world, to own our components
API::UObject* spawn_actor(API::UObject* world_context) {
    auto* statics = ue::cdo(L"Class /Script/Engine.GameplayStatics");
    auto* actor_class = ue::find_class(L"Class /Script/Engine.Actor");
    if (statics == nullptr || actor_class == nullptr) return nullptr;
    const Transform t = anim::make(Vec3{}, Rot{}, Vec3{1, 1, 1});
    API::UObject* actor = nullptr;
    // collision handling 1 = AlwaysSpawn, no owner (the Lua passed 1 and nil)
    if (!ue::call_ret(statics, L"BeginDeferredActorSpawnFromClass", {world_context, (API::UObject*)actor_class, t, (int32_t)1, (API::UObject*)nullptr}, actor) || actor == nullptr) return nullptr;
    API::UObject* finished = nullptr;
    ue::call_ret(statics, L"FinishSpawningActor", {actor, t}, finished);
    return actor;
}

// uevr_utils.create_component_of_class(class, manualAttachment = true, identity, deferredFinish = false, parent)
API::UObject* add_component(API::UObject* actor, const wchar_t* class_path) {
    auto* cls = ue::find_class(class_path);
    if (actor == nullptr || cls == nullptr) return nullptr;
    API::UObject* comp = nullptr;
    const Transform t = anim::make(Vec3{}, Rot{}, Vec3{1, 1, 1});
    if (!ue::call_ret(actor, L"AddComponentByClass", {(API::UObject*)cls, true, t, false}, comp) || comp == nullptr) return nullptr;
    ue::call(comp, L"SetVisibility", {true});
    ue::call(comp, L"SetHiddenInGame", {false});
    ue::call(comp, L"SetCollisionEnabled", {(int32_t)0});
    return comp;
}

// uevr_utils.createPoseableMeshFromSkeletalMesh with useDefaultPose (the profile sets UseDefaultPose: true)
API::UObject* make_poseable(API::UObject* actor, API::UObject* skeletal) {
    API::UObject* comp = add_component(actor, L"Class /Script/Engine.PoseableMeshComponent");
    if (comp == nullptr) return nullptr;
    if (auto** mesh = ue::prop<API::UObject*>(comp, L"SkeletalMesh")) {
        if (auto** src = ue::prop<API::UObject*>(skeletal, L"SkeletalMesh")) *mesh = *src;
    }
    // follow the game's mesh once so the bones exist, then stand alone (the Lua's Set*PoseComponent(x, true) then (nil, false))
    const wchar_t* follow = L"SetLeaderPoseComponent";
    if (!ue::call(comp, follow, {skeletal, true})) { follow = L"SetMasterPoseComponent"; ue::call(comp, follow, {skeletal, true}); }
    ue::call(comp, follow, {(API::UObject*)nullptr, false});
    // copyMaterials
    int32_t n = 0;
    ue::call_ret(skeletal, L"GetNumMaterials", {}, n);
    for (int32_t i = 0; i < n; ++i) {
        API::UObject* m = nullptr;
        if (ue::call_ret(skeletal, L"GetMaterial", {i}, m) && m != nullptr) ue::call(comp, L"SetMaterial", {i, m});
    }
    return comp;
}

// controllers.getController: the MotionControllerComponent whose MotionSource is "Left" / "Right" (controllers.lua makes them)
API::UObject* find_controller(Side side) {
    auto* cls = ue::find_class(L"Class /Script/HeadMountedDisplay.MotionControllerComponent");
    if (cls == nullptr) return nullptr;
    const std::wstring want = side == LEFT ? settings::LEFT_SOURCE : settings::RIGHT_SOURCE;
    for (auto* c : API::UObjectHook::get_objects_by_class(cls, false)) {
        auto* src = ue::prop<API::FName>(c, L"MotionSource");
        if (src != nullptr && ue::str(*src) == want) return c;
    }
    return nullptr;
}

// hands.lua createComponent + the part of create() after it, for one side
bool create_side(Side side, API::UObject* skeletal, const json::Value& def, const json::Value* initial) {
    Hand& h = g_hands[side];
    h.controller = find_controller(side);
    if (h.controller == nullptr) return false;
    h.component = make_poseable(g_actor, skeletal);
    if (h.component == nullptr) return false;
    if (auto* b = ue::prop<float>(h.component, L"BoundsScale")) *b = (float)settings::HAND_BOUNDS_SCALE;
    ue::call(h.component, L"SetCastShadow", {false});   // the Lua cleared bCastDynamicShadow: no shadow from floating arms

    bool attached = false;   // controllers.attachComponentToController: K2_AttachTo(controller, "", KeepRelativeOffset, no weld)
    ue::call_ret(h.component, L"K2_AttachTo", {h.controller, ue::name(L""), (int32_t)0, false}, attached);

    // the profile's Offset (all zero here) on top of the game mesh's own relative rotation (IgnoreRelativeOffset false)
    Rot base;
    if (auto* r = ue::prop<Rot>(skeletal, L"RelativeRotation")) base = *r;
    ue::call(h.component, L"K2_SetRelativeLocationAndRotation", {Vec3{}, base, false, false});

    const json::Value* side_def = def.get(side == LEFT ? "Left" : "Right");
    std::string joint;
    if (side_def != nullptr) if (auto* j = side_def->get("Name"); j != nullptr && j->kind == json::Value::STR) joint = j->s;
    if (!joint.empty()) {
        anim::transform_bone_to_root(h.component, std::wstring(joint.begin(), joint.end()),
                                     anim::vec3(side_def->get("Location"), Vec3{}), anim::rot(side_def->get("Rotation"), Rot{}),
                                     anim::vec3(side_def->get("Scale"), Vec3{1, 1, 1}), anim::vec3(side_def->get("TaperOffset"), Vec3{}));
    }
    int bones = 0;
    if (initial != nullptr) if (const auto* b = initial->get(side == LEFT ? "left_hand" : "right_hand")) bones = anim::initialize_bones(h.component, *b);
    if (side_def != nullptr) if (auto* a = side_def->get("AnimationID"); a != nullptr && a->kind == json::Value::STR) fingers::add(a->s, h.component);   // the finger poses
    LOGI("%s hands: %s hand made on %s, attached %s, joint %s, %d bones set from the profile", TAG, side == LEFT ? "left" : "right",
         ue::name_of(h.controller).c_str(), attached ? "yes" : "NO", joint.c_str(), bones);
    return true;
}

// main.lua regenerateHands: the forearm hands exist only when the menu's "Hands Type" is Forearms
bool forearms_chosen() {
    const json::Value* cfg = params::file("sh2_config.json");
    const json::Value* t = cfg != nullptr ? cfg->get("hands_type") : nullptr;
    return t != nullptr && t->kind == json::Value::NUM && (int)t->n == settings::HANDS_TYPE_FOREARMS;
}

bool try_create(API::UObject* pawn) {   // tryAutoCreateHands / createFromConfig, profile "Main"
    if (!forearms_chosen()) {
        if (!g_logged_wait) LOGI("%s hands: Hands Type is not Forearms in the pack's menu, so the forearm hands stay off", TAG);
        g_logged_wait = true;
        return false;
    }
    const json::Value* file = params::file("hands_parameters.json");
    const json::Value* profiles = file != nullptr ? file->get("profiles") : nullptr;
    const json::Value* profile = profiles != nullptr ? profiles->get(settings::HANDS_PROFILE) : nullptr;
    const json::Value* arms = profile != nullptr ? profile->get("Arms") : nullptr;
    if (arms == nullptr) { if (!g_logged_wait) LOGW("%s hands: no Main/Arms profile in hands_parameters.json", TAG); g_logged_wait = true; return false; }

    // "Mesh": "Pawn.Mesh": the pawn's Mesh property (uevr_utils.getObjectFromDescriptor)
    API::UObject* skeletal = nullptr;
    if (auto** m = ue::prop<API::UObject*>(pawn, L"Mesh")) skeletal = *m;
    if (skeletal == nullptr || find_controller(LEFT) == nullptr || find_controller(RIGHT) == nullptr) {
        if (!g_logged_wait) LOGI("%s hands: waiting for the pawn's mesh and both controllers", TAG);
        g_logged_wait = true;
        return false;
    }
    g_actor = spawn_actor(pawn);
    if (g_actor == nullptr) { LOGW("%s hands: could not spawn the actor that holds the hands", TAG); return false; }
    const json::Value* initial = arms->get("InitialTransform");
    const bool ok = create_side(LEFT, skeletal, *arms, initial) && create_side(RIGHT, skeletal, *arms, initial);
    if (!ok) { LOGW("%s hands: creation failed, trying again", TAG); destroy(); return false; }
    g_pawn = pawn;
    g_logged_wait = false;
    return true;
}
} // namespace

void tick(float delta) {
    API::UObject* pawn = API::get()->get_local_pawn(0);
    if (exists() && !forearms_chosen()) { LOGI("%s hands: Hands Type changed away from Forearms, taking the hands down", TAG); destroy(); }
    if (exists() && (pawn != g_pawn || !ue::alive(g_hands[LEFT].component) || !ue::alive(g_hands[RIGHT].component))) {
        LOGI("%s hands: the pawn or the hands went away, rebuilding", TAG);
        destroy();
    }
    if (exists() || pawn == nullptr) return;
    g_since_try += delta;
    if (g_since_try < settings::HANDS_RETRY_S) return;
    g_since_try = 0.0f;
    try_create(pawn);
}

bool exists() { return g_hands[LEFT].component != nullptr && g_hands[RIGHT].component != nullptr; }

const Hand& get(Side side) { return g_hands[side]; }

void hide(Side side, bool hidden) {   // hideHand
    Hand& h = g_hands[side];
    h.hidden = hidden;
    if (ue::alive(h.component)) ue::call(h.component, L"SetVisibility", {!hidden});
}

void destroy() {   // destroyHands
    for (auto& h : g_hands) {
        fingers::remove(h.component);
        if (ue::alive(h.component)) ue::call(h.component, L"K2_DestroyComponent", {h.component});
        h.component = h.controller = nullptr;
    }
    if (ue::alive(g_actor)) ue::call(g_actor, L"K2_DestroyActor", {});
    g_actor = nullptr;
    g_pawn = nullptr;
}

} // namespace sh2vr::hands
