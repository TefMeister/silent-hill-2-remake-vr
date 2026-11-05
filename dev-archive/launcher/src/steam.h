#pragma once
#include "common.h"

fs::path find_game_dir();                       // empty when Steam or the game cannot be found
bool looks_like_game_dir(const fs::path& dir);  // the shipping exe is where it should be
