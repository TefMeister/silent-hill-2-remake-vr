// selfprof.h -- how much of each frame our own plugin takes, per module (2026-11-05, the optimiser's step 1: Tefa,
// "find out what it is that actually demands so much power"). The plugin's callbacks wrap each module in a Scope;
// the heartbeat prints the per-frame average of the biggest ones and resets. Cheap: two clock reads per module.
#pragma once

#include <string>

namespace sh2vr::selfprof {

enum Slot { FIRE, BUTTONS, HOLSTERS, TWOHAND, RHBOLT, CROSSHAIR, SCREENFX, RELOAD, RACK, IK, HANDS, FINGERS, FRONTGRIP,
            GHOSTHAND, BRIDGE, VIEW, XINPUT, SLOTS };

struct Scope {
    explicit Scope(Slot s);
    ~Scope();
    Slot slot;
    long long t0;
};

std::string report(unsigned frames);   // "ours 2.31 ms/frame: ik 1.40, fingers 0.42, ..." and reset

} // namespace sh2vr::selfprof
