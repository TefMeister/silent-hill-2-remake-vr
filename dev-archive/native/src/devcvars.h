// devcvars.h -- A DEV TOOL FOR THE OPTIMISER (2026-11-10): engine settings changed while the game runs, from a file.
// If <profile>\sh2vr_devcvars.txt exists, it is read whenever it changes: one "name value" per line (# starts a comment).
// Each setting is applied through the engine's console manager and logged with its value before and after, so a
// measurement can say exactly what was on. No file: nothing happens (a player's install never has one).
// 2026-11-12: a line "uevr <Setting> <value>" sets one of the VR injector's OWN settings instead (the names in its
// config.txt, e.g. "uevr VR_RenderingMethod 3"), through its plugin interface, logged before -> after. Made so the
// alternate-eye warp can be switched on after the inject, the way its author says to, without the injector's menu.
#pragma once

namespace sh2vr::devcvars {

void tick(float delta);

} // namespace sh2vr::devcvars
