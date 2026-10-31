// input.cpp -- see input.h
#include "input.h"

#include "common.h"
#include "settings.h"

namespace sh2vr::input {

namespace {
XINPUT_STATE g_now{}, g_last{};
bool g_any = false;
bool trig(BYTE v) { return v > settings::TRIGGER_PRESSED_OVER; }
} // namespace

void on_state(const XINPUT_STATE& state) { g_now = state; g_any = true; }
void end_tick() { g_last = g_now; }

bool held(WORD button) { return g_any && (g_now.Gamepad.wButtons & button) != 0; }
bool pressed(WORD button) { return g_any && (g_now.Gamepad.wButtons & button) != 0 && (g_last.Gamepad.wButtons & button) == 0; }
bool left_trigger() { return g_any && trig(g_now.Gamepad.bLeftTrigger); }
bool right_trigger() { return g_any && trig(g_now.Gamepad.bRightTrigger); }
bool left_trigger_pressed() { return left_trigger() && !trig(g_last.Gamepad.bLeftTrigger); }
bool right_trigger_pressed() { return right_trigger() && !trig(g_last.Gamepad.bRightTrigger); }

bool grip(bool right) {
    const bool pad = held(right ? XINPUT_GAMEPAD_RIGHT_SHOULDER : XINPUT_GAMEPAD_LEFT_SHOULDER);
    static UEVR_ActionHandle action = nullptr;
    static bool asked = false;
    if (!asked) {
        asked = true;
        action = API::VR::get_action_handle(settings::VR_GRIP_ACTION);
        LOGI("%s input: UEVR's VR grip action %s: %s", TAG, settings::VR_GRIP_ACTION, action != nullptr ? "found" : "NOT found (the pad's LB / RB only)");
    }
    if (action == nullptr) return pad;
    const UEVR_InputSourceHandle src = right ? API::VR::get_right_joystick_source() : API::VR::get_left_joystick_source();
    return pad || API::VR::is_action_active(action, src);
}

} // namespace sh2vr::input
