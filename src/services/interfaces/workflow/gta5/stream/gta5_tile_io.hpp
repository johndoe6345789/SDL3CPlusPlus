#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/resource/gta5_placement.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_tile_coord.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Path of a tile file inside the package: <tilesDir>/<x>_<z>.json
std::string Gta5TilePath(const std::string& tilesDir,
                         const Gta5TileCoord& tile);

/// Read one legacy tile file, as the retired Python importer wrote them.
///
/// Returns false when the tile file does not exist, which is the normal
/// case for the sea and for any part of the map that was not exported:
/// most of the grid is empty and that is not an error.
bool ReadGta5TileFile(const std::string& path,
                      std::vector<Gta5Placement>& outPlacements,
                      const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
