// config_lines_test.cpp -- the launcher's config.txt line setter (src/config_lines.h) on made-up text
// (SH2-032: the safe start values written before every inject). bash dev-archive/native/tests/run.sh runs it.
#include <cstdio>

#include "../src/config_lines.h"

namespace {
int g_fail = 0;
void check(bool ok, const char* what) { std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what); if (!ok) ++g_fail; }
}  // namespace

int main() {
    using config_lines::set;
    const std::vector<std::pair<std::wstring, std::wstring>> safe = {
        {L"Frontend_RequestedRuntime", L"openxr_loader.dll"}, {L"VR_GhostingFix", L"false"}, {L"VR_RenderingMethod", L"0"}};

    // what a normal quit leaves behind (2026-11-22): both crash values, Windows line ends
    std::vector<config_lines::Change> ch;
    std::wstring after = set(L"VR_AFW_FramewarpMode=3\r\nVR_GhostingFix=true\r\nVR_RenderingMethod=3\r\n"
                             L"Frontend_RequestedRuntime=openxr_loader.dll\r\nVR_WorldScale=1.0\r\n", safe, ch);
    check(after == L"VR_AFW_FramewarpMode=3\nVR_GhostingFix=false\nVR_RenderingMethod=0\n"
                   L"Frontend_RequestedRuntime=openxr_loader.dll\nVR_WorldScale=1.0\n",
          "the two crash values are set back in place; every other line is kept");
    check(ch.size() == 2 && ch[0].key == L"VR_GhostingFix" && ch[0].was == L"true" && ch[1].key == L"VR_RenderingMethod" &&
          ch[1].was == L"3", "the two changes are reported, with what was there");

    // already safe: nothing changes, nothing reported
    ch.clear();
    std::wstring same = set(after, safe, ch);
    check(same == after && ch.empty(), "a file that is already safe stays the same and reports nothing");

    // a missing line is added at the end
    ch.clear();
    std::wstring added = set(L"VR_WorldScale=1.0\n", safe, ch);
    check(added == L"VR_WorldScale=1.0\nFrontend_RequestedRuntime=openxr_loader.dll\nVR_GhostingFix=false\nVR_RenderingMethod=0\n",
          "missing lines are added at the end");
    check(ch.size() == 3 && ch[0].was.empty(), "an added line is reported with nothing before it");

    // a key that only starts like another is not touched
    ch.clear();
    std::wstring look = set(L"VR_GhostingFixStrength=5\nVR_GhostingFix=true\n", {{L"VR_GhostingFix", L"false"}}, ch);
    check(look == L"VR_GhostingFixStrength=5\nVR_GhostingFix=false\n" && ch.size() == 1,
          "VR_GhostingFixStrength is left alone (only the exact key= is matched)");

    std::printf("%s\n", g_fail ? "config_lines_test: FAILED" : "config_lines_test: all passed");
    return g_fail ? 1 : 0;
}
