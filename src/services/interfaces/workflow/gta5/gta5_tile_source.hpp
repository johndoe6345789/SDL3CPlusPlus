#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// A tile's placements from whichever source is live: the extracted map
/// through the asset index when gta5.assets.index has run, and otherwise
/// assets/tiles/<x>_<z>.json under `tilesDir`.
void ReadGta5TilePlacements(const Gta5StreamState& state,
                            const std::string& tilesDir,
                            const Gta5TileCoord& tile,
                            std::vector<Gta5Placement>& out,
                            const std::shared_ptr<ILogger>& logger);

/// Read a newly wanted tile: its placements, the band it spawns at, and
/// every archetype it needs handed to the load pool at once.
void ReadGta5ResidentTile(Gta5StreamState& state, const std::string& tilesDir,
                          const Gta5TileCoord& tile,
                          Gta5ResidentTile& resident,
                          const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
