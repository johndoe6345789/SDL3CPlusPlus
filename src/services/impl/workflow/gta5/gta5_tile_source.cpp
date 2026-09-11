#include "services/interfaces/workflow/gta5/gta5_tile_source.hpp"

#include "services/interfaces/workflow/gta5/gta5_geometry_request.hpp"
#include "services/interfaces/workflow/gta5/gta5_grid.hpp"
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

void ReadGta5ResidentTile(Gta5StreamState& state, const std::string& tilesDir,
                          const Gta5TileCoord& tile,
                          Gta5ResidentTile& resident,
                          const std::shared_ptr<ILogger>& logger) {
    // Marked read even when empty, so an empty tile is not searched again
    // every frame.
    ReadGta5TilePlacements(state, tilesDir, tile, resident.placements, logger);
    resident.placementsRead = true;
    const float distance = glm::distance(Gta5TileCentre(state.world, tile),
                                         state.centreOrigin);
    resident.bandAtSpawn = Gta5BandForDistance(state.world, distance);
    PrefetchGta5Tile(state, resident);
    if (logger && !resident.placements.empty()) {
        logger->Info("gta5.tiles.load: tile " + std::to_string(tile.x) + "_" +
                     std::to_string(tile.z) + " has " +
                     std::to_string(resident.placements.size()) +
                     " placements");
    }
}

}  // namespace sdl3cpp::services::impl
