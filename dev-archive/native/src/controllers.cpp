// controllers.cpp -- see controllers.h
#include "controllers.h"

#include "settings.h"
#include "ue.h"

namespace sh2vr::controllers {

API::UObject* find(Side side) {   // controllers.getController: the one whose MotionSource is "Left" / "Right"
    auto* cls = ue::find_class(L"Class /Script/HeadMountedDisplay.MotionControllerComponent");
    if (cls == nullptr) return nullptr;
    const std::wstring want = side == LEFT ? settings::LEFT_SOURCE : settings::RIGHT_SOURCE;
    for (auto* c : API::UObjectHook::get_objects_by_class(cls, false)) {
        auto* src = ue::prop<API::FName>(c, L"MotionSource");
        if (src != nullptr && ue::str(*src) == want) return c;
    }
    return nullptr;
}

} // namespace sh2vr::controllers
