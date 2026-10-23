// json_mini.h -- a small JSON reader for the Hands Off saved files (2026-10-15). The ported features keep Andyalpa's
// files (written by REFramework's json.dump_file); the menu Lua writes them, the plugin reads them back. Read-only, no
// third-party library: objects, arrays, strings (basic escapes), numbers, true/false/null.
#pragma once
#include <map>
#include <string>
#include <vector>

namespace sh2vr::json {

struct Value {
    enum Kind { NUL, BOOL, NUM, STR, ARR, OBJ } kind = NUL;
    bool b = false;
    double n = 0.0;
    std::string s;
    std::vector<Value> arr;
    std::map<std::string, Value> obj;

    const Value* get(const std::string& key) const {   // nullptr when not an object or the key is missing
        if (kind != OBJ) return nullptr;
        auto it = obj.find(key);
        return it == obj.end() ? nullptr : &it->second;
    }
};

bool parse(const std::string& text, Value& out);       // false on malformed input (out is then partial)
bool load_file(const std::wstring& path, Value& out);  // false when missing or malformed

} // namespace sh2vr::json
