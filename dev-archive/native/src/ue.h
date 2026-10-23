// ue.h -- calling the game's own functions by name from C++: `component:SetVisibility(true)` in the Lua pack is a
// UFunction call whose parameters UEVR's Lua binding filled in by reflection. This does the same for C++: find the
// function on the object's class, lay the arguments out by the function's own parameter offsets, call it, copy the
// return value back. Every ported module goes through here, so a wrong argument is refused with one log line
// instead of handed to the game (the lesson of the Village crash, 2026-10-22).
#pragma once

#include <cstdint>
#include <initializer_list>
#include <string_view>

#include "common.h"

namespace sh2vr::ue {

// Unreal 5 keeps these in doubles. Sizes checked below: a wrong size here would corrupt a parameter block.
struct Vec3 { double x = 0, y = 0, z = 0; };                 // FVector
struct Rot { double pitch = 0, yaw = 0, roll = 0; };         // FRotator
struct Quat { double x = 0, y = 0, z = 0, w = 1; };          // FQuat
struct alignas(16) Transform {                               // FTransform: rotation, translation, scale, 16-aligned
    Quat rot;
    Vec3 pos; double pad0 = 0;
    Vec3 scale{1, 1, 1}; double pad1 = 0;
};
static_assert(sizeof(Vec3) == 24 && sizeof(Rot) == 24 && sizeof(Quat) == 32 && sizeof(Transform) == 96);

#pragma warning(push)
#pragma warning(disable : 4324)   // padded for Transform's 16-byte alignment: intended
struct Arg {
    enum Kind { BOOL, INT, FLOAT, DOUBLE, OBJECT, NAME, VEC3, ROT, TRANSFORM } kind;
    bool b = false; int32_t i = 0; float f = 0; double d = 0; API::UObject* o = nullptr; API::FName name;
    Vec3 v; Rot r; Transform t;
    Arg(bool x) : kind(BOOL), b(x) {}
    Arg(int32_t x) : kind(INT), i(x) {}
    Arg(float x) : kind(FLOAT), f(x) {}
    Arg(double x) : kind(DOUBLE), d(x) {}
    Arg(API::UObject* x) : kind(OBJECT), o(x) {}
    Arg(API::FName x) : kind(NAME), name(x) {}
    Arg(Vec3 x) : kind(VEC3), v(x) {}
    Arg(Rot x) : kind(ROT), r(x) {}
    Arg(Transform x) : kind(TRANSFORM), t(x) {}
};
#pragma warning(pop)

// obj->fn(args...). False (with one log line per function) when the function is missing, an argument does not
// match its parameter, or the return value does not fit `ret_size`. `ret` receives the ReturnValue bytes.
bool call(API::UObject* obj, std::wstring_view fn, std::initializer_list<Arg> args, void* ret = nullptr, size_t ret_size = 0);
template <typename T>
bool call_ret(API::UObject* obj, std::wstring_view fn, std::initializer_list<Arg> args, T& out) {
    return call(obj, fn, args, &out, sizeof out);
}

std::string name_of(API::UObject* o);   // the object's full name in UTF-8, or "null"

API::UClass* find_class(std::wstring_view path);    // "Class /Script/Engine.Actor"; cached; nullptr if not loaded
API::UObject* cdo(std::wstring_view class_path);    // a class's default object: where static functions are called
bool alive(API::UObject* o);                        // still a live object UEVR knows about
API::FName name(std::wstring_view s);               // an FName from text
std::wstring str(const API::FName& n);              // and back
bool is_none(const API::FName& n);                  // the "None" name GetParentBone returns past the root

// a property in place on an object, or nullptr (no write past a missing property)
template <typename T>
T* prop(API::UObject* o, std::wstring_view name) { return o != nullptr ? o->get_property_data<T>(name) : nullptr; }

} // namespace sh2vr::ue
