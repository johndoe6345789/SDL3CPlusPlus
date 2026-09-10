#pragma once

#include "services/interfaces/workflow/graphics/texture_image_io.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// The GPU texture created for one loaded image, plus the mip chain length
/// computed for it (floor(log2(max(w,h))) + 1).
struct UploadedTexture {
    SDL_GPUTexture* texture = nullptr;
    Uint32 numLevels        = 1;
};

/**
 * @brief Creates a mip-chained GPU texture and uploads `image` into its
 * base level, generating the rest of the mip chain on the GPU.
 *
 * Always frees `image.pixels` (whether upload succeeds or a later step
 * fails) since texture.load never needs the CPU copy again after the
 * transfer buffer holds it. Throws std::runtime_error on any GPU resource
 * creation failure, releasing whatever GPU resources were already created.
 */
UploadedTexture UploadTextureImage(SDL_GPUDevice* device,
                                   LoadedTextureImage& image);

}  // namespace sdl3cpp::services::impl
