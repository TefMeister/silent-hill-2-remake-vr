// tuning.h -- THE RENDERING VALUES THE VR BUILD RUNS WITH (2026-11-13).
// A short table of engine settings that make the frame cheaper without a difference Tefa could see in the headset
// (2026-11-13: switched on and off live three times, "i see no change ... i could not see it"): ray-traced reflections
// traced at half size and on the shinier surfaces only, the bounce light's probes half as dense, and two distance-field
// helpers ray tracing makes redundant. Measured with the frame warp on, real headset: 18.1 -> 15.85 ms in the armory.
// Applied once the player's character exists and looked at again every few seconds (the game writes its own values
// back when it loads a level or applies its settings). A file sh2vr_notune.txt in the profile keeps the game's values.
#pragma once

namespace sh2vr::tuning {

void tick(float delta);

} // namespace sh2vr::tuning
