#include "services/interfaces/workflow/gta5/gta5_tile_source.hpp"

#include "services/interfaces/workflow/gta5/gta5_geometry_request.hpp"
#include "services/interfaces/workflow/gta5/gta5_grid.hpp"
#include "services/interfaces/workflow/gta5/gta5_tile_io.hpp"
#include "services/interfaces/workflow/gta5/gta5_ymap_tile.hpp"

#include <chrono>
#include <exception>
#include <utility>

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

void StartGta5TileRead(const Gta5StreamState& state,
                       const std::string& tilesDir, const Gta5TileCoord& tile,
                       Gta5ResidentTile& resident,
                       const std::shared_ptr<ILogger>& logger) {
    if (!state.assets) {
        // Legacy tile files are small JSON: read here, handed over ready.
        std::vector<Gta5Placement> out;
        ReadGta5TilePlacements(state, tilesDir, tile, out, logger);
        std::promise<std::vector<Gta5Placement>> ready;
        ready.set_value(std::move(out));
        resident.reading = ready.get_future();
        return;
    }
    // The index never changes once built, and ymaps are read straight
    // from their files, so the thread shares nothing that is written.
    resident.reading = std::async(
        std::launch::async, [assets = state.assets, world = state.world, tile] {
            std::vector<Gta5Placement> out;
            ReadGta5YmapTile(*assets, world, tile, out);
            return out;
        });
}

bool TakeGta5TileRead(Gta5StreamState& state, const Gta5TileCoord& tile,
                      Gta5ResidentTile& resident,
                      const std::shared_ptr<ILogger>& logger) {
    if (!resident.reading.valid() ||
        resident.reading.wait_for(std::chrono::seconds(0)) !=
            std::future_status::ready) {
        return false;
    }
    try {
        resident.placements = resident.reading.get();
    } catch (const std::exception& ex) {
        if (logger) {
            logger->Warn(std::string("gta5.tiles.load: read failed: ") +
                         ex.what());
        }
        resident.placements.clear();
    }
    // Marked read even when empty, so an empty tile is not searched again.
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
    return true;
}

}  // namespace sdl3cpp::services::impl
