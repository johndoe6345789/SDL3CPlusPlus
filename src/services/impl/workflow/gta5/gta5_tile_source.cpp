#include "services/interfaces/workflow/gta5/gta5_tile_source.hpp"

#include "services/interfaces/workflow/gta5/gta5_tile_io.hpp"
#include "services/interfaces/workflow/gta5/gta5_ymap_tile.hpp"

namespace sdl3cpp::services::impl {

void ReadGta5TilePlacements(const Gta5StreamState& state,
                            const std::string& tilesDir,
                            const Gta5TileCoord& tile,
                            std::vector<Gta5Placement>& out,
                            const std::shared_ptr<ILogger>& logger) {
    if (state.assets) {
        ReadGta5YmapTile(*state.assets, state.world, tile, out);
        return;
    }
    // A missing tile file is normal out at sea: most of the grid is
    // empty, and that is not an error.
    ReadGta5TileFile(Gta5TilePath(tilesDir, tile), out, logger);
}

}  // namespace sdl3cpp::services::impl
