// upscaler_text.h -- the pure text part of the upscaler check (upscaler.cpp), so a test can run it on made-up files
// (dev-archive/launcher/tests/upscaler_test.cpp).
//
// Why: on 2026-11-10 the game had rewritten its own GameUserSettings.ini (October: DLSS Quality, custom; found that
// day: the upscaler off, preset High, motion blur on) and drew every pixel at full size: the headset's 30 a second
// (modding-notes/2026-11-10-optimiser-step-3-the-upscaler-was-off.md). The numbers, read from the file then:
//   Adv_Supersampling 4 = DLSS (0 = off)       Adv_DLSSQuality 3 = Quality, 2 = Balanced
//   Adv_MotionBlur 0 = off                     IngameMotionBlur=False
// The rule (Tefa: Silent Hill 2 VR is played with DLSS on): the upscaler OFF or another one -> DLSS, at Quality;
// DLSS already on -> the player's own quality is kept. Motion blur always off (every blur off in VR).
#pragma once

#include <string>
#include <vector>

namespace upscaler {

inline constexpr int DLSS = 4;
inline constexpr int DLSS_QUALITY = 3;
inline constexpr int MOTION_BLUR_OFF = 0;

// the number after `("<key>", ` in the GFXSettings line, or -1 when the key is not there
inline int read_opt(const std::string& text, const std::string& key) {
    const std::string pat = "(\"" + key + "\", ";
    const size_t at = text.find(pat);
    if (at == std::string::npos) return -1;
    size_t i = at + pat.size();
    bool neg = false;
    if (i < text.size() && text[i] == '-') { neg = true; ++i; }
    int v = 0, digits = 0;
    while (i < text.size() && text[i] >= '0' && text[i] <= '9') { v = v * 10 + (text[i++] - '0'); ++digits; }
    return digits == 0 ? -1 : neg ? -v : v;
}

inline bool write_opt(std::string& text, const std::string& key, int value) {
    const std::string pat = "(\"" + key + "\", ";
    const size_t at = text.find(pat);
    if (at == std::string::npos) return false;
    const size_t start = at + pat.size();
    const size_t end = text.find(')', start);
    if (end == std::string::npos) return false;
    text.replace(start, end - start, std::to_string(value));
    return true;
}

// puts the file's text right; each change is described in `said` (plain words for the launcher's window)
inline std::string fix(std::string text, std::vector<std::string>& said) {
    const int ss = read_opt(text, "Adv_Supersampling");
    if (ss >= 0 && ss != DLSS) {
        write_opt(text, "Adv_Supersampling", DLSS);
        write_opt(text, "Adv_DLSSQuality", DLSS_QUALITY);
        said.push_back(ss == 0 ? "the game had switched its upscaler OFF (it does that by itself now and then): DLSS is back on, at Quality"
                               : "the game's upscaler was not DLSS (setting " + std::to_string(ss) + "): DLSS is back on, at Quality");
    }
    const int mb = read_opt(text, "Adv_MotionBlur");
    if (mb > MOTION_BLUR_OFF) {
        write_opt(text, "Adv_MotionBlur", MOTION_BLUR_OFF);
        said.push_back("motion blur was on in the game's settings: off again");
    }
    const std::string on = "IngameMotionBlur=True";
    if (const size_t at = text.find(on); at != std::string::npos) {
        text.replace(at, on.size(), "IngameMotionBlur=False");
        if (mb <= MOTION_BLUR_OFF) said.push_back("motion blur was on in the game's settings: off again");
    }
    return text;
}

}  // namespace upscaler
