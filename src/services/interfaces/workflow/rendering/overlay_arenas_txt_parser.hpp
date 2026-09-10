#pragma once

#include "services/interfaces/workflow/quake3/q3_overlay_utils.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Parses scripts/arenas.txt out of the map's pk3 into a lowercase-keyed
/// map name -> {longname, first bot} table for the map-select menu.
q3overlay::ArenaMap ParseArenasTxt(const std::string& pk3);

}  // namespace sdl3cpp::services::impl
