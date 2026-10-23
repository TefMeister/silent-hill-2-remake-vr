cmake_minimum_required(VERSION 3.21)
project(sh2vr LANGUAGES CXX)

# sh2vr.dll: our UEVR C++ plugin for Silent Hill 2 (2024). Started 2026-10-23 as the skeleton the Lua pack is
# ported into one module at a time (modding-notes/2026-10-23-cpp-rewrite-plan.md). Static runtime, like the
# other Visceral plugins, so the game folder needs no extra runtime files.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

file(GLOB SH2VR_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(sh2vr SHARED ${SH2VR_SOURCES})
target_include_directories(sh2vr PRIVATE include src)
target_compile_definitions(sh2vr PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN WINVER=0x0A00 _WIN32_WINNT=0x0A00)
# /W4 for our code; 4100 and 4189 come from UEVR's own Plugin.hpp (unused parameters in its empty callbacks)
target_compile_options(sh2vr PRIVATE /W4 /wd4100 /wd4189 /EHsc /MP /utf-8)
set_target_properties(sh2vr PROPERTIES OUTPUT_NAME "sh2vr" PREFIX "")
