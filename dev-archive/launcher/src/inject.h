#pragma once
#include "common.h"

#include <cstdint>

// Starting the game through Steam and loading UEVR into it: the same steps as praydog's uevr-frontend
// (MIT; UEVR/MainWindow.xaml.cs + Injector.cs): optional plugin nullifier, the VR runtime's loader, the
// profile's Frontend_RequestedRuntime line, then UEVRBackend.dll, each by LoadLibraryW in a remote thread.
struct InjectOptions {
    bool openvr = false;     // default OpenXR (Virtual Desktop, the OpenXR Simulator)
    bool nullify = false;    // also run UEVR's VR-plugin nullifier first
    uint32_t delay_ms = 0;   // wait after the window shows
};

uint32_t find_game_pid();                                   // 0 when not running
bool launch_through_steam();
uint32_t wait_for_game(uint32_t timeout_ms);                // pid or 0
bool wait_for_window(uint32_t pid, uint32_t timeout_ms);
bool module_loaded(uint32_t pid, const wchar_t* name);
bool inject_uevr(uint32_t pid, const Places& pl, const InjectOptions& o);
