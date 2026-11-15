// upscaler_test.cpp -- the shipped text fix (src/upscaler_text.h) on made-up settings lines, plus one copy of a real
// file if given (argv[1]): bash dev-archive/native/tests/run.sh builds and runs it with the other tests.
#include <cstdio>
#include <fstream>
#include <sstream>

#include "../src/upscaler_text.h"

namespace {
int g_fail = 0;
void check(bool ok, const char* what) { std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what); if (!ok) ++g_fail; }
std::string line(int ss, int q, int mb, const char* imb) {
    return "GFXSettings=(GsaOptions=((\"Quality\", -1),(\"Adv_MotionBlur\", " + std::to_string(mb) + "),(\"Adv_Supersampling\", " + std::to_string(ss) +
           "),(\"Adv_DLSSQuality\", " + std::to_string(q) + "),(\"Adv_XeSSQuality\", 2)),Quality=High,IngameMotionBlur=" + imb + ")\r\n";
}
}  // namespace

int main(int argc, char** argv) {
    using namespace upscaler;
    std::vector<std::string> said;

    const std::string good = line(4, 2, 0, "False");   // DLSS on at Balanced: the player's choice, kept
    check(fix(good, said) == good && said.empty(), "DLSS on (any level), blur off: the file is left exactly as it is");

    said.clear();
    const std::string reset = fix(line(0, 2, 1, "True"), said);   // what the game did by itself on 2026-11-10
    check(read_opt(reset, "Adv_Supersampling") == 4, "upscaler off -> DLSS");
    check(read_opt(reset, "Adv_DLSSQuality") == 3, "... at Quality");
    check(read_opt(reset, "Adv_MotionBlur") == 0 && reset.find("IngameMotionBlur=False") != std::string::npos, "motion blur off, both places");
    check(read_opt(reset, "Quality") == -1 && read_opt(reset, "Adv_XeSSQuality") == 2, "nothing else touched (preset -1, XeSS 2)");
    check(said.size() == 2, "two plain lines said (upscaler, blur)");
    check(reset.size() >= 2 && reset.substr(reset.size() - 2) == "\r\n", "line endings kept");

    said.clear();
    const std::string fsr = fix(line(2, 3, 0, "False"), said);
    check(read_opt(fsr, "Adv_Supersampling") == 4 && said.size() == 1, "another upscaler (2) -> DLSS, said once");

    said.clear();
    check(fix("[ScalabilityGroups]\r\nsg.ShadowQuality=3\r\n", said) == "[ScalabilityGroups]\r\nsg.ShadowQuality=3\r\n" && said.empty(),
          "a file with no graphics line: left alone");

    if (argc > 1) {   // a real file: read only here, never written
        std::ifstream f(argv[1], std::ios::binary);
        std::ostringstream s;
        s << f.rdbuf();
        said.clear();
        const std::string real = s.str(), out = fix(real, said);
        std::printf("real file: Adv_Supersampling %d, Adv_DLSSQuality %d, Adv_MotionBlur %d -> %s\n", read_opt(real, "Adv_Supersampling"),
                    read_opt(real, "Adv_DLSSQuality"), read_opt(real, "Adv_MotionBlur"), out == real ? "left as it is" : "would be put right");
        check(out.size() + 8 >= real.size() && out.size() <= real.size() + 8, "real file: only a few characters can change");
    }
    std::printf(g_fail ? "%d FAILED\n" : "all passed\n", g_fail);
    return g_fail ? 1 : 0;
}
