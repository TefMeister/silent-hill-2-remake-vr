"""wire_realcam.py : one-off, puts realcam into settings.h, plugin.cpp and cutscreen.cpp (run once, 2026-11-12)."""
import os
d = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "native", "src") + os.sep
def rw(name, fn):
    s = open(d + name, encoding="utf-8", newline="").read(); t = fn(s); assert t != s, name
    open(d + name, "w", encoding="utf-8", newline="").write(t)
def once(s, old, new):
    assert s.count(old) == 1, old; return s.replace(old, new)
rw("settings.h", lambda s: once(s, "constexpr float CUTSCENE_SCREEN_DISTANCE = 3.0f;",
    "constexpr float CUTSCENE_SCREEN_DISTANCE = 3.0f;\n// 2026-11-12 (realcam.cpp): while flat, the cutscene camera's own view angle and one picture for both eyes\nconstexpr bool CUTSCENE_REAL_CAMERA = true;"))
def plugin(p):
    p = once(p, '#include "cutscreen.h"', '#include "cutscreen.h"\n#include "realcam.h"')
    p = once(p, "cutscreen::tick(delta); }", "cutscreen::tick(delta); realcam::tick(delta); }")
    old = "    void on_post_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int, float, UEVR_Vector3f*, UEVR_Rotatorf*, bool) override { selfprof::Scope sp_(selfprof::VIEW); twohand::on_view(); rhbolt::on_view(); }"
    new = ("    void on_pre_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle device, int, float, UEVR_Vector3f* position, UEVR_Rotatorf*, bool is_double) override { if (is_double) realcam::on_pre_view((void*)device, (void*)position); }\n"
           "    void on_post_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int, float, UEVR_Vector3f* position, UEVR_Rotatorf*, bool) override { selfprof::Scope sp_(selfprof::VIEW); realcam::on_post_view((void*)position); twohand::on_view(); rhbolt::on_view(); }")
    return once(p, old, new)
rw("plugin.cpp", plugin)
def cut(c):
    c = once(c, '#include "common.h"', '#include "common.h"\n#include "realcam.h"')
    a = '            vr->set_mod_value(KEY_FLAT, "true");'
    c = once(c, a, a + "\n            realcam::set_active(true);")
    b = '            vr->set_mod_value(KEY_FLAT, "false");'
    return once(c, b, "            realcam::set_active(false);\n" + b)
rw("cutscreen.cpp", cut)
print("wired")
