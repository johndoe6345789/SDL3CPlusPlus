#include "services/interfaces/workflow/fs2024/terrain/fs2024_vegetation_gpu.hpp"

#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

namespace sdl3cpp::services::impl {

void ReleaseUnusedFs2024Vegetation(SDL_GPUDevice* device,
                                   Fs2024TileStreamState& state) {
    std::unordered_map<std::string, bool> used;
    for (const auto& [key, tile] : state.resident) {
        for (const auto& chunk : tile.vegetationChunks) {
            used[chunk.speciesName] = true;
        }
    }
    for (auto it = state.vegetationSpecies.begin();
         it != state.vegetationSpecies.end();) {
        if (used.count(it->first)) {
            ++it;
            continue;
        }
        SDL_ReleaseGPUTexture(device, it->second.texture);
        SDL_ReleaseGPUSampler(device, it->second.sampler);
        it = state.vegetationSpecies.erase(it);
    }
}

}  // namespace sdl3cpp::services::impl
