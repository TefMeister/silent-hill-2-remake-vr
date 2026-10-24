// crosshair.h -- no crosshair, ever (Tefa 2026-10-24: "please remove the crosshair, we will not use it in game at all").
// The game draws its aim crosshair as a HUD widget, CrosshairContainer_WidgetBP (the class jbusfield's reticule finder
// saved in reticule_parameters.json). The pack's 3D reticule is hidden ("uevr_reticule_hide": true), and a hidden
// reticule never takes the game's widget off the screen (reticule.lua createFromWidget returns before
// RemoveFromViewport), so UEVR shows it on the HUD plane in front of the face, where it follows the head. This
// collapses every instance and takes it out of the viewport, and keeps it that way.
#pragma once

namespace sh2vr::crosshair {

void find(float delta);   // pre-engine tick: refreshes the list of crosshair widgets now and then
void hide();              // post-engine tick: after the game's own tick, before the HUD is painted
int shown_again();        // how many times the game made a crosshair visible again (for the heartbeat)

} // namespace sh2vr::crosshair
