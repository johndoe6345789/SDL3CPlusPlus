#pragma once

#include <SDL3/SDL_gpu.h>

#include <string>

namespace sdl3cpp::services::impl {

/// Expands a leading `~` in `raw` to `$HOME`, exactly as texture.load
/// always has (a no-op when `HOME` isn't set).
std::string ResolveTextureImagePath(const std::string& raw);

/// One image decoded by stb_image, forced to 4 (RGBA) channels.
struct LoadedTextureImage {
    unsigned char* pixels = nullptr;
    int width  = 0;
    int height = 0;
};

/// Decodes `path` with stb_image; throws std::runtime_error (prefixed
/// "texture.load: ...", including stb's failure reason) if it can't.
LoadedTextureImage LoadTextureImagePixels(const std::string& path);

/// Frees `image.pixels` (a no-op if already null) and nulls it out.
void FreeTextureImagePixels(LoadedTextureImage& image);

/// The GPU texture created for one loaded image, plus the mip chain length
/// computed for it (floor(log2(max(w,h))) + 1).
struct UploadedTexture {
    SDL_GPUTexture* texture = nullptr;
    Uint32 numLevels = 1;
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

/// Creates the linear/anisotropic/repeat sampler texture.load always used,
/// sized to `numLevels`. Throws std::runtime_error (and releases `texture`)
/// if creation fails.
SDL_GPUSampler* CreateTextureLoadSampler(SDL_GPUDevice* device,
                                         SDL_GPUTexture* texture,
                                         Uint32 numLevels);

}  // namespace sdl3cpp::services::impl
