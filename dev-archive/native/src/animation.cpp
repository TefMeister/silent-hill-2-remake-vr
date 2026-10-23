// animation.cpp -- see animation.h. Function names of jbusfield's animation.lua are kept in the comments.
#include "animation.h"

#include "settings.h"

namespace sh2vr::anim {

namespace {
API::UObject* kml() { return ue::cdo(L"Class /Script/Engine.KismetMathLibrary"); }
} // namespace

Transform make(Vec3 location, Rot rotation, Vec3 scale) {
    Transform t;
    ue::call_ret(kml(), L"MakeTransform", {location, rotation, scale}, t);
    return t;
}

Transform compose(const Transform& a, const Transform& b) {
    Transform t;
    ue::call_ret(kml(), L"ComposeTransforms", {a, b}, t);
    return t;
}

Transform invert(const Transform& t) {
    Transform out;
    ue::call_ret(kml(), L"InvertTransform", {t}, out);
    return out;
}

API::FName parent_bone(API::UObject* comp, const API::FName& bone) {
    API::FName out;
    ue::call_ret(comp, L"GetParentBone", {bone}, out);
    return out;
}

API::FName root_of(API::UObject* comp, const API::FName& bone) {   // getRootBoneOfBone
    API::FName at = bone, last = bone;
    for (int guard = 0; !ue::is_none(at) && guard < settings::MAX_BONE_DEPTH; ++guard) {
        last = at;
        at = parent_bone(comp, at);
    }
    return last;
}

Transform bone_transform(API::UObject* comp, const API::FName& bone) {
    Transform t;
    ue::call_ret(comp, L"GetBoneTransformByName", {bone, (int32_t)WORLD_SPACE}, t);
    return t;
}

void set_local_transform(API::UObject* comp, const API::FName& bone, const Transform& local, const Transform* parent) {   // setBoneSpaceLocalTransform
    const Transform p = parent != nullptr ? *parent : bone_transform(comp, parent_bone(comp, bone));
    const Transform world = compose(local, p);
    ue::call(comp, L"SetBoneTransformByName", {bone, world, (int32_t)WORLD_SPACE});
}

Transform local_transform(API::UObject* comp, const API::FName& bone) {   // getBoneSpaceLocalTransform
    const Transform p = bone_transform(comp, parent_bone(comp, bone));
    return compose(bone_transform(comp, bone), invert(p));
}

void set_local_rotator(API::UObject* comp, const API::FName& bone, Rot local) {   // setBoneSpaceLocalRotator
    const Transform p = bone_transform(comp, parent_bone(comp, bone));
    Rot world;
    if (!ue::call_ret(kml(), L"TransformRotation", {p, local}, world)) return;
    ue::call(comp, L"SetBoneRotationByName", {bone, world, (int32_t)WORLD_SPACE});
}

void transform_bone_to_root(API::UObject* comp, const std::wstring& target, Vec3 location, Rot rotation, Vec3 scale, Vec3 taper) {
    const API::FName target_name = ue::name(target);
    const Transform root = bone_transform(comp, root_of(comp, target_name));
    const API::FName parent = parent_bone(comp, target_name);
    const Vec3 tiny{settings::HIDDEN_BONE_SCALE, settings::HIDDEN_BONE_SCALE, settings::HIDDEN_BONE_SCALE};

    // every bone not in the target's branch shrinks to nothing at the root (the Lua's loop runs index 1..count)
    const Transform collapsed = make(Vec3{}, Rot{}, tiny);
    int32_t count = 0;
    ue::call_ret(comp, L"GetNumBones", {}, count);
    for (int32_t i = 1; i <= count; ++i) {
        API::FName child;
        if (!ue::call_ret(comp, L"GetBoneName", {i}, child) || ue::is_none(child)) continue;
        bool in_branch = false;
        ue::call_ret(comp, L"BoneIsChildOf", {child, parent}, in_branch);
        if (!in_branch) set_local_transform(comp, child, collapsed, &root);
    }
    // the bone above the target tapers to a point at location + taper
    set_local_transform(comp, parent, make(Vec3{location.x + taper.x, location.y + taper.y, location.z + taper.z}, Rot{}, tiny), &root);
    set_local_transform(comp, target_name, make(location, rotation, scale), &root);
}

int initialize_bones(API::UObject* comp, const json::Value& bones) {   // initializeBones
    if (bones.kind != json::Value::OBJ) return 0;
    int n = 0;
    for (const auto& [bone_name, parts] : bones.obj) {
        const API::FName bone = ue::name(std::wstring(bone_name.begin(), bone_name.end()));
        const Transform current = local_transform(comp, bone);
        Rot current_rot;
        ue::call_ret(kml(), L"TransformRotation", {current, Rot{}}, current_rot);
        const Vec3 location = vec3(parts.get("location"), current.pos);
        const Rot rotation = rot(parts.get("rotation"), current_rot);
        const Vec3 scale = vec3(parts.get("scale"), current.scale);
        set_local_transform(comp, bone, make(location, rotation, scale));
        ++n;
    }
    return n;
}

Vec3 vec3(const json::Value* v, Vec3 fallback) {
    if (v == nullptr || v->kind != json::Value::ARR || v->arr.size() < 3) return fallback;
    return Vec3{v->arr[0].n, v->arr[1].n, v->arr[2].n};
}

Rot rot(const json::Value* v, Rot fallback) {
    if (v == nullptr || v->kind != json::Value::ARR || v->arr.size() < 3) return fallback;
    return Rot{v->arr[0].n, v->arr[1].n, v->arr[2].n};
}

} // namespace sh2vr::anim
