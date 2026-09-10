#pragma once

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// A square RGBA8 atlas of a BSP's lightmap blocks, laid out on a
/// `gridSize` x `gridSize` grid of LM_BLOCK_SIZE-pixel tiles. Slot 0 is
/// solid white, for faces with lm_index == -1.
struct LightmapAtlas {
    std::vector<uint8_t> pixels;
    int width        = 0;
    int height       = 0;
    int gridSize     = 0;
    int numLightmaps = 0;
};

/// Builds the lightmap atlas from a loaded BSP's LUMP_LIGHTMAPS, applying
/// a x4 overbright scale to each RGB texel (matching Q3's lightmap
/// brightness convention).
LightmapAtlas BuildLightmapAtlas(const std::vector<uint8_t>& bspData);

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
