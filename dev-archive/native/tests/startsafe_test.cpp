// startsafe_test.cpp -- the plugin's config.txt line setter (src/startsafe_text.h) on made-up text (SH2-032).
// bash dev-archive/native/tests/run.sh runs it.
#include <cstdio>

#include "../src/startsafe_text.h"

namespace {
int g_fail = 0;
void check(bool ok, const char* what) { std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what); if (!ok) ++g_fail; }
}  // namespace

int main() {
    using sh2vr::startsafe_text::set;
    using sh2vr::startsafe_text::Change;
    const std::vector<std::pair<std::string, std::string>> safe = {{"VR_GhostingFix", "false"}, {"VR_RenderingMethod", "0"}};

    std::vector<Change> ch;
    const std::string quit = "VR_AFW_FramewarpMode=3\r\nVR_GhostingFix=true\r\nVR_RenderingMethod=3\r\nVR_WorldScale=1.0\r\n";
    const std::string fixed = set(quit, safe, ch);
    check(fixed == "VR_AFW_FramewarpMode=3\r\nVR_GhostingFix=false\r\nVR_RenderingMethod=0\r\nVR_WorldScale=1.0\r\n",
          "what a run leaves behind: both crash values put back, Windows line ends and every other line kept");
    check(ch.size() == 2 && ch[0].was == "true" && ch[1].was == "3", "both changes reported with what was there");

    ch.clear();
    check(set(fixed, safe, ch) == fixed && ch.empty(), "an already safe file is not touched");

    ch.clear();
    check(set("VR_GhostingFix=true", safe, ch) == "VR_GhostingFix=false" && ch.size() == 1, "a last line with no line end");

    ch.clear();
    check(set("VR_GhostingFixStrength=5\nVR_RenderingMethodX=3\n", safe, ch) == "VR_GhostingFixStrength=5\nVR_RenderingMethodX=3\n" &&
              ch.empty(), "keys that only start like ours are left alone");

    ch.clear();
    check(set("VR_WorldScale=1.0\n", safe, ch) == "VR_WorldScale=1.0\n" && ch.empty(),
          "a missing key is not added (UEVR's default is safe)");

    std::printf("%s\n", g_fail ? "startsafe_test: FAILED" : "startsafe_test: all passed");
    return g_fail ? 1 : 0;
}
