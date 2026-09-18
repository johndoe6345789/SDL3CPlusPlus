#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_finish_ready.hpp"

#include "services/interfaces/workflow/fs2024/tiles/fs2024_load_pool.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_finish.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

#include <algorithm>
#include <chrono>

namespace sdl3cpp::services::impl {
namespace {

using Clock = std::chrono::steady_clock;

double MsSince(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start)
        .count();
}

}  // namespace

void FinishFs2024ReadyTiles(Fs2024TileStreamState& state,
                            SDL_GPUDevice* device,
                            btDiscreteDynamicsWorld* physics,
                            double budgetMs, ILogger* logger,
                            Fs2024FinishStats& stats) {
    const Clock::time_point frame = Clock::now();
    for (int taken = 0;
         state.pool && (taken == 0 || MsSince(frame) < budgetMs); ++taken) {
        auto ready = state.pool->Take(1);
        if (ready.empty()) return;
        Fs2024PreparedTile& tile = ready.front();
        state.loading.erase(tile.key);
        if (!tile.error.empty()) {
            state.missing.insert(tile.key);
            if (logger) {
                logger->Warn("fs2024.tiles.load: tile " +
                             std::to_string(tile.key.level) + "/" +
                             std::to_string(tile.key.x) + "/" +
                             std::to_string(tile.key.z) + " failed: " +
                             tile.error);
            }
            continue;
        }
        if (!state.wanted.count(tile.key) || state.resident.count(tile.key)) {
            continue;
        }
        const Clock::time_point start = Clock::now();
        state.resident.emplace(
            tile.key, FinishFs2024Tile(device, physics, *state.world, tile,
                                       state.landmarkKits,
                                       state.vegetationSpecies));
        ++stats.finished;
        stats.worstMs = std::max(stats.worstMs, MsSince(start));
    }
}

}  // namespace sdl3cpp::services::impl
