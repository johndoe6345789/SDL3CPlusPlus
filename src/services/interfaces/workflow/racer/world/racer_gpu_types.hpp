#pragma once

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One uploaded material batch: an unindexed triangle list.
struct RacerGpuBatch {
    SDL_GPUBuffer* vertices = nullptr;
    std::uint32_t vertexCount = 0;
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    bool blended = false;  ///< intensity decals (shadows, glows)
};

struct RacerGpuModel {
    std::vector<RacerGpuBatch> batches;
};

/// A GPU texture made from one material, upscaled once and shared.
struct RacerGpuTexture {
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
};

}  // namespace sdl3cpp::services::impl
