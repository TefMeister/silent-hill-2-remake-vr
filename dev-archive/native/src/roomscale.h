// roomscale.h -- ROOMSCALE MOVEMENT STEPS ASIDE WHILE THE GAME STEERS JAMES (2026-11-13).
// Tefa, real headset: with the injector's roomscale movement on, the head's place in the room moves James's body. In a
// cutscene he slid around with the headset (seen while the game was paused) and jittered while it played (the cutscene
// places him, the head pulls him away). Roomscale off by hand: both gone [verified-live 2026-11-13, Tefa]. Tefa: it must
// also step aside in vaults and interactions "where the game takes control away from me".
// The signal: the player controller's IsMoveInputIgnored() (true in the opening cutscene, false while walking
// [measured 2026-11-13]) or the game's own "in a cutscene". IsPlayingRootMotion() is NO signal: this game walks by root
// motion, it reads true all the time. Added the same day: James not the view target (memory spots) and the movement
// component in custom mode 6 (squeezes, crawls, vaults: the game's traversals). Each switch is logged with its reason.
#pragma once

namespace sh2vr::roomscale {

void tick(float delta);

} // namespace sh2vr::roomscale
