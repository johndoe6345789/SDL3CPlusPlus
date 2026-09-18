#pragma once

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_blocks.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// Uploads one of FS2024's landmark colour maps (DXT1 or DXT5), its
/// blocks passed through compressed with every mip, with a repeating
/// sampler. UNORM, not sRGB: the shader decodes, as it does for every
/// fs2024 colour map. An empty binding when the device refuses it.
SDL_GPUTextureSamplerBinding UploadFs2024LandmarkTexture(
    SDL_GPUDevice* device, const sdl3cpp::fs2024::DdsBlocks& dds);

}  // namespace sdl3cpp::services::impl
