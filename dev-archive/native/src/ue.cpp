// ue.cpp -- see ue.h
#include "ue.h"

#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace sh2vr::ue {

namespace {
std::set<std::wstring> g_refused;   // one log line per function, not one per frame

bool refuse(std::wstring_view fn, const char* why) {
    if (g_refused.insert(std::wstring(fn)).second) LOGW("%s call refused: %s: %s", TAG, narrow(fn).c_str(), why);
    return false;
}

// the parameter's class name ("IntProperty", "StructProperty" ...) and, for a struct, the struct's name
std::wstring class_of(API::FProperty* p) {
    auto* c = p->get_class();
    return c != nullptr ? c->get_name() : L"";
}
std::wstring struct_of(API::FProperty* p) {
    auto* s = ((API::FStructProperty*)p)->get_struct();
    return s != nullptr ? s->get_fname()->to_string() : L"";
}

// writes one argument at its parameter's offset; false when the kinds do not match
bool put(uint8_t* buf, API::FProperty* p, const Arg& a) {
    const std::wstring cls = class_of(p);
    uint8_t* at = buf + p->get_offset();
    if (cls == L"BoolProperty") {
        if (a.kind != Arg::BOOL) return false;
        const uint32_t mask = ((API::FBoolProperty*)p)->get_byte_mask();
        if (a.b) *at |= (uint8_t)(mask != 0 ? mask : 1);
        return true;
    }
    if (cls == L"IntProperty") { if (a.kind != Arg::INT) return false; std::memcpy(at, &a.i, 4); return true; }
    if (cls == L"ByteProperty" || cls == L"EnumProperty") { if (a.kind != Arg::INT) return false; *at = (uint8_t)a.i; return true; }
    if (cls == L"FloatProperty") {
        const float f = a.kind == Arg::FLOAT ? a.f : a.kind == Arg::DOUBLE ? (float)a.d : a.kind == Arg::INT ? (float)a.i : 0.0f;
        if (a.kind != Arg::FLOAT && a.kind != Arg::DOUBLE && a.kind != Arg::INT) return false;
        std::memcpy(at, &f, 4); return true;
    }
    if (cls == L"DoubleProperty") {
        const double d = a.kind == Arg::DOUBLE ? a.d : a.kind == Arg::FLOAT ? (double)a.f : a.kind == Arg::INT ? (double)a.i : 0.0;
        if (a.kind != Arg::FLOAT && a.kind != Arg::DOUBLE && a.kind != Arg::INT) return false;
        std::memcpy(at, &d, 8); return true;
    }
    if (cls == L"ObjectProperty" || cls == L"ClassProperty" || cls == L"InterfaceProperty") {
        if (a.kind != Arg::OBJECT) return false; std::memcpy(at, &a.o, sizeof a.o); return true;
    }
    if (cls == L"NameProperty") { if (a.kind != Arg::NAME) return false; std::memcpy(at, &a.name, sizeof a.name); return true; }
    if (cls == L"StructProperty") {
        const std::wstring s = struct_of(p);
        if (s == L"Vector" && a.kind == Arg::VEC3) { std::memcpy(at, &a.v, sizeof a.v); return true; }
        if (s == L"Rotator" && a.kind == Arg::ROT) { std::memcpy(at, &a.r, sizeof a.r); return true; }
        if (s == L"Transform" && a.kind == Arg::TRANSFORM) { std::memcpy(at, &a.t, sizeof a.t); return true; }
        return false;
    }
    return false;   // strings, arrays, delegates: not needed yet; add when a module needs one
}

// the byte size the return value occupies, for the fit check
size_t size_of(API::FProperty* p) {
    const std::wstring cls = class_of(p);
    if (cls == L"BoolProperty" || cls == L"ByteProperty" || cls == L"EnumProperty") return 1;
    if (cls == L"IntProperty" || cls == L"FloatProperty") return 4;
    if (cls == L"DoubleProperty" || cls == L"ObjectProperty" || cls == L"ClassProperty" || cls == L"NameProperty") return 8;
    if (cls == L"StructProperty") {
        const std::wstring s = struct_of(p);
        if (s == L"Vector" || s == L"Rotator") return 24;
        if (s == L"Quat") return 32;
        if (s == L"Transform") return 96;
    }
    return 0;
}
} // namespace

bool call(API::UObject* obj, std::wstring_view fn, std::initializer_list<Arg> args, void* ret, size_t ret_size) {
    if (obj == nullptr) return false;
    auto* cls = obj->get_class();
    auto* f = cls != nullptr ? cls->find_function(fn) : nullptr;
    if (f == nullptr) return refuse(fn, "no such function on this object's class");
    const int32_t size = f->get_properties_size();
    std::vector<uint8_t> buf((size_t)std::max<int32_t>(size, 1), 0);

    API::FProperty* ret_prop = nullptr;
    size_t i = 0;
    for (auto* fld = f->get_child_properties(); fld != nullptr; fld = fld->get_next()) {
        auto* p = (API::FProperty*)fld;
        if (!p->is_param()) continue;
        if (p->is_return_param()) { ret_prop = p; continue; }
        if (p->is_out_param() && !p->is_reference_param()) continue;   // written by the function; not read back yet
        if (i >= args.size()) return refuse(fn, "fewer arguments than parameters");
        if (!put(buf.data(), p, *(args.begin() + i))) return refuse(fn, "an argument does not match its parameter");
        ++i;
    }
    if (i != args.size()) return refuse(fn, "more arguments than parameters");
    if (ret != nullptr) {
        if (ret_prop == nullptr) return refuse(fn, "it returns nothing, a return value was asked for");
        if (size_of(ret_prop) != ret_size) return refuse(fn, "the return value is a different size than asked for");
    }
    obj->process_event(f, buf.data());
    if (ret != nullptr) {
        std::memcpy(ret, buf.data() + ret_prop->get_offset(), ret_size);
        if (class_of(ret_prop) == L"BoolProperty") {   // a bool return may be a bit in a byte
            const uint32_t mask = ((API::FBoolProperty*)ret_prop)->get_byte_mask();
            *(uint8_t*)ret = (*(uint8_t*)ret & (uint8_t)(mask != 0 ? mask : 1)) != 0 ? 1 : 0;
        }
    }
    return true;
}

std::string name_of(API::UObject* o) { return o != nullptr ? narrow(o->get_full_name()) : "null"; }

} // namespace sh2vr::ue
