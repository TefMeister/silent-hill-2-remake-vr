// animation.h -- bone work on a PoseableMeshComponent, ported from jbusfield's animation.lua (2026-10-23).
// The Lua did its transform maths through KismetMathLibrary; so does this, so the numbers come out the same.
// A library, not a module: no state beyond what the caller passes in (design note, 2026-10-23c).
#pragma once

#include <string>

#include "json_mini.h"
#include "ue.h"

namespace sh2vr::anim {

using ue::Rot;
using ue::Transform;
using ue::Vec3;

constexpr int WORLD_SPACE = 0;   // EBoneSpaces::WorldSpace, the space every Lua call used (boneSpace = 0)

Transform make(Vec3 location, Rot rotation, Vec3 scale);       // KismetMathLibrary.MakeTransform
Transform compose(const Transform& a, const Transform& b);      // ComposeTransforms(a, b): a, then b
Transform invert(const Transform& t);                           // InvertTransform

API::FName parent_bone(API::UObject* comp, const API::FName& bone);
API::FName root_of(API::UObject* comp, const API::FName& bone);   // getRootBoneOfBone
Transform bone_transform(API::UObject* comp, const API::FName& bone);

// setBoneSpaceLocalTransform: `local` is relative to `parent` (the parent bone's transform when not given)
void set_local_transform(API::UObject* comp, const API::FName& bone, const Transform& local, const Transform* parent = nullptr);
Transform local_transform(API::UObject* comp, const API::FName& bone);   // getBoneSpaceLocalTransform
void set_local_rotator(API::UObject* comp, const API::FName& bone, Rot local);   // setBoneSpaceLocalRotator

// transformBoneToRoot (parentPathOnly = false): every bone outside `target`'s branch collapses to the root, the bone
// above `target` gets the taper, `target` itself gets location / rotation / scale relative to the root.
void transform_bone_to_root(API::UObject* comp, const std::wstring& target, Vec3 location, Rot rotation, Vec3 scale, Vec3 taper);

// initializeBones: per bone {"location": [x,y,z], "rotation": [p,y,r], "scale": [x,y,z]}, each part optional
int initialize_bones(API::UObject* comp, const json::Value& bones);

Vec3 vec3(const json::Value* v, Vec3 fallback);   // [x, y, z] from json, or the fallback
Rot rot(const json::Value* v, Rot fallback);      // [pitch, yaw, roll]

} // namespace sh2vr::anim
