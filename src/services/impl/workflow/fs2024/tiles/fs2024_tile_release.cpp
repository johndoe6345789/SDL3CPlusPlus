#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_release.hpp"

#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_upload.hpp"

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

}  // namespace sdl3cpp::services::impl
