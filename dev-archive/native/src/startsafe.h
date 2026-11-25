// startsafe.h -- the injector's two crash-prone settings put back to their safe start values in config.txt the moment
// the plugin is loaded (2026-11-25, SH2-032).
//
// The plugin switches VR_GhostingFix and VR_RenderingMethod on a few seconds into the headset's drawing (ghostfix.cpp,
// warp.cpp). UEVR writes the live values into config.txt whenever it saves: just after its start-up, and each time its
// menu opens or closes (praydog/UEVR master, Framework::save_config callers, read 2026-11-25) -- so after such a run the
// file holds both, and a start with either one crashes (measured twice on 2026-11-22).
//
// When it is done: UEVR loads every plugin DLL in its Framework constructor (PluginLoader::early_init) BEFORE it first
// reads config.txt (Mods::on_initialize_d3d_thread -> reload_config, then the plugins' initialize, then reload_config
// again). A setting changed only in memory at the plugin's initialize would therefore be read back from the file; the
// FILE is corrected instead, from a static initializer of this DLL, which runs while the DLL is being loaded. Only
// kernel32 calls there (the loader lock is held). What was changed is logged later, from on_initialize.
// Our launcher writes the same values before every inject as well (launcher/src/inject.cpp).
#pragma once

namespace sh2vr::startsafe {

void log_what_was_done();   // from on_initialize: one log line per value put back (or "already safe"), and the live values

} // namespace sh2vr::startsafe
