#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_release.hpp"

#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_upload.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_load_pool.hpp"

#include <limits>

namespace sdl3cpp::services::impl {

void ReleaseFs2024Tile(SDL_GPUDevice* device, btDiscreteDynamicsWorld* world,
                       Fs2024LoadedTile& tile) {
    ReleaseFs2024TerrainChunks(device, tile.terrain);
    RemoveFs2024TerrainCollision(world, tile.terrain.collision);
    if (device) {
        SDL_ReleaseGPUTexture(device, tile.classMap);
        SDL_ReleaseGPUSampler(device, tile.classSampler);
        SDL_ReleaseGPUBuffer(device, tile.buildingChunk.vertexBuffer);
        SDL_ReleaseGPUBuffer(device, tile.buildingChunk.indexBuffer);
        SDL_ReleaseGPUBuffer(device, tile.buildingRoofChunk.vertexBuffer);
        SDL_ReleaseGPUBuffer(device, tile.buildingRoofChunk.indexBuffer);
    }
}

void StopFs2024Loads(Fs2024TileStreamState& state) {
    if (state.pool) {
        state.pool->Drop([](const Fs2024TileKey&) { return false; });
        state.pool->WaitIdle();
        state.pool->Take(std::numeric_limits<std::size_t>::max());
    }
    state.loading.clear();
}

void ReleaseAllFs2024Tiles(SDL_GPUDevice* device,
                           btDiscreteDynamicsWorld* world,
                           Fs2024TileStreamState& state) {
    StopFs2024Loads(state);
    for (auto& [key, tile] : state.resident) {
        ReleaseFs2024Tile(device, world, tile);
    }
    state.resident.clear();
    state.drawn.clear();
    state.wanted.clear();
    state.loading.clear();
    state.pendingLoad.clear();
    state.pendingEvict.clear();
    state.missing.clear();
}

}  // namespace sdl3cpp::services::impl
