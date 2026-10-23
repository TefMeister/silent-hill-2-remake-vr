// common.h -- logging and the small shared pieces every sh2vr file uses.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "uevr/API.hpp"

namespace sh2vr {

using API = uevr::API;
constexpr const char* TAG = "[sh2vr]";

#define SH2_LOG(fn, ...) do { auto& api_ = ::sh2vr::API::get(); if (api_) api_->fn(__VA_ARGS__); } while (0)   // get() is a unique_ptr
#define LOGI(...) SH2_LOG(log_info, __VA_ARGS__)
#define LOGW(...) SH2_LOG(log_warn, __VA_ARGS__)
#define LOGE(...) SH2_LOG(log_error, __VA_ARGS__)

std::string narrow(std::wstring_view w);     // UTF-16 to UTF-8, for log lines
std::wstring appdata_dir();                  // %APPDATA%, no trailing slash

} // namespace sh2vr
