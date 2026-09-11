#pragma once

#include "services/interfaces/workflow/gta5/gta5_asset_index.hpp"
#include "services/interfaces/workflow/gta5/gta5_config_types.hpp"
#include "services/interfaces/workflow/gta5/gta5_placement.hpp"
#include "services/interfaces/workflow/gta5/gta5_tile_coord.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// The placements of one tile, read from the ymaps with entities in it.
///
/// A ymap straddling tiles is read once per tile, and each tile keeps
/// only the entities inside it, so nothing is placed twice. This is what
/// assets/tiles/<x>_<z>.json used to hold, read when the tile is wanted.
void ReadGta5YmapTile(const Gta5AssetIndex& index,
                      const Gta5WorldConfig& world, const Gta5TileCoord& tile,
                      std::vector<Gta5Placement>& out);

}  // namespace sdl3cpp::services::impl
