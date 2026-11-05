cmake_minimum_required(VERSION 3.21)
project(sh2vr_launcher LANGUAGES CXX)

# SH2VR.exe: installs the Silent Hill 2 VR mod, resets what must be vanilla, starts the game and loads UEVR into it.
# Started 2026-11-05 (modding-notes/2026-11-05-uevr-inside-our-mod-and-the-launcher.md). Static runtime: no extra files.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

file(GLOB SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(SH2VR ${SOURCES})
target_compile_definitions(SH2VR PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE WINVER=0x0A00 _WIN32_WINNT=0x0A00)
target_compile_options(SH2VR PRIVATE /W4 /EHsc /MP /utf-8)
target_link_libraries(SH2VR PRIVATE shell32 ole32 advapi32)
