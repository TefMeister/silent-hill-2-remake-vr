// common.cpp -- see common.h
#include "common.h"

#include <windows.h>
#include <shlobj.h>

namespace sh2vr {

std::string narrow(std::wstring_view w) {
    if (w.empty()) return "";
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(n > 0 ? n : 0, '\0');
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), out.data(), n, nullptr, nullptr);
    return out;
}

std::wstring appdata_dir() {
    wchar_t* p = nullptr;
    std::wstring out;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &p)) && p != nullptr) out = p;
    if (p != nullptr) CoTaskMemFree(p);
    return out;
}

} // namespace sh2vr
