#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

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

/// Start reading a newly wanted tile on a thread of its own: parsing a
/// tile's ymaps took 4-9 ms, more than a whole frame at 240 Hz.
void StartGta5TileRead(const Gta5StreamState& state,
                       const std::string& tilesDir, const Gta5TileCoord& tile,
                       Gta5ResidentTile& resident,
                       const std::shared_ptr<ILogger>& logger);

/// Adopt a finished read: its placements, the band it spawns at, and
/// every archetype it needs handed to the load pool at once. False while
/// it is still being read.
bool TakeGta5TileRead(Gta5StreamState& state, const Gta5TileCoord& tile,
                      Gta5ResidentTile& resident,
                      const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
