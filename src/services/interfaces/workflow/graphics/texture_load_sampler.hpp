#pragma once

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// Creates the linear/anisotropic/repeat sampler texture.load always used,
/// sized to `numLevels`. Throws std::runtime_error (and releases `texture`)
/// if creation fails.
SDL_GPUSampler* CreateTextureLoadSampler(SDL_GPUDevice* device,
                                         SDL_GPUTexture* texture,
                                         Uint32 numLevels);

}  // namespace sdl3cpp::services::impl
