#pragma once

#include <SDL3/SDL_gpu.h>

#include <stdint.h>
#include <string>
#include <vector>

namespace sdl3cpp::q3 {

/// Creates a GPU buffer of @p usage and uploads @p size bytes of @p data.
SDL_GPUBuffer* UploadMd3Buffer(SDL_GPUDevice* device,
                               SDL_GPUBufferUsageFlags usage, const void* data,
                               uint32_t size);

/// Creates an RGBA8 sampler texture from tightly packed @p pixels.
SDL_GPUTexture* UploadMd3Texture(SDL_GPUDevice* device, const uint8_t* pixels,
                                 int width, int height);

/// Linear/repeat sampler used by every MD3 surface.
SDL_GPUSampler* MakeMd3LinearSampler(SDL_GPUDevice* device);

/// Decodes and uploads the first @p candidates entry that exists in @p pk3.
SDL_GPUTexture* TryLoadMd3Texture(SDL_GPUDevice* device, const std::string& pk3,
                                  const std::vector<std::string>& candidates);

}  // namespace sdl3cpp::q3
