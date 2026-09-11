#pragma once

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// Creates the linear/anisotropic/repeat sampler texture.load always used,
/// sized to `numLevels`. `mipLodBias` above 0 picks smaller mips -- less
/// aliasing on textures without authored mips, but softer. Throws
/// std::runtime_error (and releases `texture`) if creation fails.
SDL_GPUSampler* CreateTextureLoadSampler(SDL_GPUDevice* device,
                                         SDL_GPUTexture* texture,
                                         Uint32 numLevels,
                                         float mipLodBias = 0.5f);

}  // namespace sdl3cpp::services::impl
