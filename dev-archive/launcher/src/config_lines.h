// config_lines.h -- set "Key=value" lines in UEVR's config.txt text (header-only, no Windows calls, so it is tested
// on made-up text by tests/config_lines_test.cpp).
//
// 2026-11-25 (SH2-032): the plugin switches VR_GhostingFix and VR_RenderingMethod on a few seconds after the headset
// starts drawing (ghostfix.cpp, warp.cpp); UEVR writes those live values into config.txt at a normal quit, and a start
// with either one crashes (measured twice on 2026-11-22). The launcher therefore writes the safe start values together
// with the runtime line, just before UEVR is injected and reads the file.
#pragma once
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace config_lines {

struct Change {
    std::wstring key, was, now;   // was = L"" when the line was missing and added
};

// Returns the text with every key set to its value: an existing line is replaced in place, a missing one is added at
// the end. Other lines are kept as they are (a trailing '\r' is dropped; the file is written with '\n'). Every line
// that really changed is reported in `changes`.
inline std::wstring set(const std::wstring& text, const std::vector<std::pair<std::wstring, std::wstring>>& wanted,
                        std::vector<Change>& changes) {
    std::wistringstream in(text);
    std::wstring line, out;
    std::vector<bool> found(wanted.size(), false);
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        for (size_t i = 0; i < wanted.size(); ++i) {
            const std::wstring head = wanted[i].first + L"=";
            if (line.rfind(head, 0) == 0) {
                const std::wstring was = line.substr(head.size());
                if (was != wanted[i].second) changes.push_back({wanted[i].first, was, wanted[i].second});
                line = head + wanted[i].second;
                found[i] = true;
                break;
            }
        }
        out += line + L"\n";
    }
    for (size_t i = 0; i < wanted.size(); ++i) {
        if (found[i]) continue;
        out += wanted[i].first + L"=" + wanted[i].second + L"\n";
        changes.push_back({wanted[i].first, L"", wanted[i].second});
    }
    return out;
}

}  // namespace config_lines
