#pragma once
#include "common.h"

// Before every start (game closed): the game's own graphics settings checked, and put back when the game switched its
// upscaler off or motion blur on by itself. The file is backed up first. Rules and why: upscaler_text.h.
bool check_upscaler(const Places& pl);   // false only when the file could not be written back
