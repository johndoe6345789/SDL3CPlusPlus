#pragma once

#include "services/interfaces/workflow/graphics/texture_image_io.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/**
 * @brief Uploads `image.pixels` into `texture`'s base mip level via a
 *        transfer buffer, then generates the rest of its `numLevels`-1
 *        mip chain on the GPU.
 *
 * Always frees `image.pixels` before returning (success or throw), since
 * texture.load never needs the CPU copy again once the transfer buffer
 * holds it.
 *
 * @throws std::runtime_error (releasing `texture`) if the transfer buffer
 *         can't be created.
 */
void UploadPixelsToTexture(SDL_GPUDevice* device, SDL_GPUTexture* texture,
                           LoadedTextureImage& image, Uint32 numLevels);

}  // namespace sdl3cpp::services::impl
