#pragma once
#include "common.h"

// Places our package (profile, paks), puts the settings that must be vanilla back to vanilla, and can take it
// all out again. Every file it adds, replaces or moves is written to the install record first, and every file it
// replaces or moves is copied to a backup, so uninstall puts the PC back exactly as it was.
bool install_all(const Places& pl);    // safe to run every launch: copies only what changed
bool reset_vanilla(const Places& pl);  // the player's own button changes -> backed up, removed
bool uninstall_all(const Places& pl);
void print_status(const Places& pl);
