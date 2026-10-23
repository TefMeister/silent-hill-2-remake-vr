// json_mini.cpp -- see json_mini.h
#include "json_mini.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace sh2vr::json {

namespace {
struct Parser {
    const std::string& t;
    size_t i = 0;
    int depth = 0;
    static constexpr int MAX_DEPTH = 64;

    void ws() { while (i < t.size() && (t[i] == ' ' || t[i] == '\t' || t[i] == '\n' || t[i] == '\r')) ++i; }
    bool lit(const char* w) {
        size_t k = 0;
        while (w[k] != '\0') { if (i + k >= t.size() || t[i + k] != w[k]) return false; ++k; }
        i += k;
        return true;
    }
    bool str(std::string& out) {
        if (i >= t.size() || t[i] != '"') return false;
        ++i;
        while (i < t.size() && t[i] != '"') {
            char c = t[i++];
            if (c == '\\' && i < t.size()) {
                const char e = t[i++];
                switch (e) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'u': i += 4; c = '?'; break;   // weapon ids and keys are plain ASCII; a \u escape is kept as '?'
                default: c = e; break;               // \" \\ \/
                }
            }
            out += c;
        }
        if (i >= t.size()) return false;
        ++i;
        return true;
    }
    bool value(Value& v) {
        if (++depth > MAX_DEPTH) return false;
        ws();
        bool ok = false;
        if (i >= t.size()) ok = false;
        else if (t[i] == '{') ok = object(v);
        else if (t[i] == '[') ok = array(v);
        else if (t[i] == '"') { v.kind = Value::STR; ok = str(v.s); }
        else if (lit("true")) { v.kind = Value::BOOL; v.b = true; ok = true; }
        else if (lit("false")) { v.kind = Value::BOOL; v.b = false; ok = true; }
        else if (lit("null")) { v.kind = Value::NUL; ok = true; }
        else {
            const char* start = t.c_str() + i;
            char* end = nullptr;
            v.n = std::strtod(start, &end);
            ok = end != start;
            if (ok) { v.kind = Value::NUM; i += (size_t)(end - start); }
        }
        --depth;
        return ok;
    }
    bool object(Value& v) {
        v.kind = Value::OBJ;
        ++i;
        ws();
        if (i < t.size() && t[i] == '}') { ++i; return true; }
        while (i < t.size()) {
            ws();
            std::string key;
            if (!str(key)) return false;
            ws();
            if (i >= t.size() || t[i] != ':') return false;
            ++i;
            if (!value(v.obj[key])) return false;
            ws();
            if (i < t.size() && t[i] == ',') { ++i; continue; }
            if (i < t.size() && t[i] == '}') { ++i; return true; }
            return false;
        }
        return false;
    }
    bool array(Value& v) {
        v.kind = Value::ARR;
        ++i;
        ws();
        if (i < t.size() && t[i] == ']') { ++i; return true; }
        while (i < t.size()) {
            v.arr.emplace_back();
            if (!value(v.arr.back())) return false;
            ws();
            if (i < t.size() && t[i] == ',') { ++i; continue; }
            if (i < t.size() && t[i] == ']') { ++i; return true; }
            return false;
        }
        return false;
    }
};
} // namespace

bool parse(const std::string& text, Value& out) {
    Parser p{text};
    return p.value(out);
}

bool load_file(const std::wstring& path, Value& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::stringstream ss;
    ss << f.rdbuf();
    return parse(ss.str(), out);
}

} // namespace sh2vr::json
