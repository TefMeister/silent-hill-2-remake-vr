// screenfx.h -- every blur and screen effect off while the game runs in VR (2026-11-05; the standing rule, Tefa
// 2026-10-12: "switch off any blur effects in VR, also any screen effects ... so there is only gameplay during gameplay").
// The plugin only loads with UEVR, so flat play is never touched. The engine's own console settings are set every few
// seconds if the game put them back (its graphics menu does), each change logged once.
#pragma once

namespace sh2vr::screenfx {

void tick(float delta);

} // namespace sh2vr::screenfx
