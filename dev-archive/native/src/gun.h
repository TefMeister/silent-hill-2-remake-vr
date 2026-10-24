// gun.h -- the gun in hand and the things around it that every reload module asks for (2026-10-24, the RE8-recipe
// port): the ranged combat part, the equipped weapon and its mesh, a socket's place, the two controllers, the headset,
// a buzz. All in the game's world units (centimetres) unless said otherwise.
#pragma once

#include <string_view>

#include "common.h"
#include "controllers.h"
#include "ue.h"

namespace sh2vr::gun {

using ue::Rot;
using ue::Transform;
using ue::Vec3;

API::UObject* pawn();
API::UObject* ranged_part();                 // SHRangedCmbSubcomp of the pawn's Combat component
API::UObject* equipped();                    // GetEquippedRangedWeapon, or nullptr
bool is_pistol(API::UObject* weapon);        // WeaponPistol_BP_C
API::UObject* mesh(API::UObject* weapon);    // the weapon's skeletal mesh component ("Mesh")

bool socket_loc(API::UObject* mesh, std::wstring_view socket, Vec3& out);
bool socket_xform(API::UObject* mesh, std::wstring_view socket, Transform& out);   // world space

bool controller(controllers::Side side, Vec3& loc, Rot& rot);   // the MotionControllerComponent's world pose
bool hmd(Vec3& loc, Rot& rot);                                 // the player camera (the headset in VR)
bool hmd_active();

void buzz(controllers::Side side, float amplitude, float seconds);

Rot rot_of(const Transform& t);   // the transform's rotation as a rotator (KismetMathLibrary)

} // namespace sh2vr::gun
