#pragma once

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

/// One species' whole imposter atlas on the GPU, untouched (BC7, every
/// layer its own variant) -- shared by every tile that stands it.
struct Fs2024VegSpeciesGpu {
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
};

/// One species' billboards for one tile: a merged mesh (every instance
/// and variation of it in this tile), naming which of
/// Fs2024TileStreamState::vegetationSpecies to sample.
struct Fs2024VegetationChunkGpu {
    std::string speciesName;
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    std::uint32_t indexCount = 0;
};

}  // namespace sdl3cpp::services::impl
