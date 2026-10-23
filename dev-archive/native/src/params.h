// params.h -- jbusfield's saved parameter files (data/*_parameters.json in the UEVR profile), read once at start and
// again whenever one changes on disk. The Lua pack writes them from its config menu; the C++ reads the same files, so
// nothing is retyped and the menu keeps working until it is ported last (plan, decision 3).
#pragma once

#include <string>
#include <vector>

#include "json_mini.h"

namespace sh2vr::params {

// the file names jbusfield's pack keeps in data/ (without the folder)
inline const std::vector<std::string> FILES = {
    "hands_parameters.json", "attachments_parameters.json", "accessories_parameters.json", "ik_parameters.json",
    "input_parameters.json", "montage_parameters.json", "pawn_parameters.json", "reticule_parameters.json",
    "collision_parameters.json", "remap_parameters.json", "ui_parameters.json", "gunstock_parameters.json",
};

void load_all();                                // read every file; logs one line per file (keys, or why not)
void watch();                                   // call once a frame: re-reads a file whose write time changed
const json::Value* file(const std::string& name);   // the parsed file, or nullptr if it never loaded
int loaded_count();

} // namespace sh2vr::params
