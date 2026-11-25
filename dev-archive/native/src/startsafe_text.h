// startsafe_text.h -- set "Key=value" lines in UEVR's config.txt text (header-only, no Windows calls, so
// tests/startsafe_test.cpp checks it on made-up text). The narrow-text twin of the launcher's config_lines.h.
#pragma once
#include <string>
#include <vector>

namespace sh2vr::startsafe_text {

struct Change {
    std::string key, was, now;
};

// Every existing "key=..." line of `wanted` gets its value; other lines (and their '\r\n' or '\n' ends) are kept as
// they are. A missing key is NOT added: UEVR's default is used then, which is safe for both keys.
inline std::string set(const std::string& text, const std::vector<std::pair<std::string, std::string>>& wanted,
                       std::vector<Change>& changes) {
    std::string out;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t end = text.find('\n', pos);
        const size_t next = end == std::string::npos ? text.size() : end + 1;
        size_t body_end = end == std::string::npos ? text.size() : end;
        if (body_end > pos && text[body_end - 1] == '\r') --body_end;
        std::string body = text.substr(pos, body_end - pos);
        const std::string tail = text.substr(body_end, next - body_end);
        for (const auto& kv : wanted) {
            const std::string head = kv.first + "=";
            if (body.rfind(head, 0) == 0) {
                const std::string was = body.substr(head.size());
                if (was != kv.second) {
                    changes.push_back({kv.first, was, kv.second});
                    body = head + kv.second;
                }
                break;
            }
        }
        out += body + tail;
        pos = next;
    }
    return out;
}

} // namespace sh2vr::startsafe_text
