#pragma once

#include "services/interfaces/workflow/rendering/bsp_lightmap_atlas_build.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// GPU handles for an uploaded LightmapAtlas.
struct LightmapAtlasGpu {
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
};

/// Uploads `atlas` to a GPU texture (submitting its own command buffer)
/// and creates a clamp-to-edge linear sampler for it.
LightmapAtlasGpu UploadLightmapAtlas(SDL_GPUDevice* device,
                                     const LightmapAtlas& atlas);

}  // namespace sdl3cpp::services::impl
