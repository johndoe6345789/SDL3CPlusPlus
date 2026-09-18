#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_resolve.hpp"

#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_plan.hpp"

namespace sdl3cpp::services::impl {

void Fs2024ResolveWantedTiles(Fs2024TileStreamState& state,
                              const glm::vec3& viewer) {
    const std::vector<Fs2024TileKey> wanted =
        SelectFs2024Tiles(viewer, state.tileSize, state.lod);
    state.wanted = {wanted.begin(), wanted.end()};

    std::unordered_set<Fs2024TileKey> resident;
    for (const auto& [key, tile] : state.resident) resident.insert(key);
    Fs2024TilePlan plan = PlanFs2024Tiles(wanted, resident, state.missing);
    state.drawn = std::move(plan.draw);
    state.pendingEvict = std::move(plan.evict);
    state.pendingLoad.clear();
    for (const Fs2024TileKey& key : plan.load) {
        if (!state.loading.count(key)) state.pendingLoad.push_back(key);
    }
    for (auto it = state.missing.begin(); it != state.missing.end();) {
        it = state.wanted.count(*it) ? std::next(it) : state.missing.erase(it);
    }
}

}  // namespace sdl3cpp::services::impl
