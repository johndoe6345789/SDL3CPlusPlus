#pragma once

#include <SDL3/SDL_gpu.h>

#include <map>
#include <string>

// Forward-declared to avoid pulling <zip.h> into every includer.
typedef struct zip zip_t;

namespace sdl3cpp::services::impl {

/// A texture and its sampler, uploaded to the GPU with a full mip chain.
struct BspTextureUpload {
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    /// True if the pk3 entry was found via `shaderImages` rather than a name
    /// matching `texName` directly.
    bool viaShader = false;
};

/**
 * @brief Finds, decodes and uploads a BSP texture's image from a pk3.
 *
 * Tries `texName` first, then (if `shaderImages` maps it to a different
 * image) that image, each against .jpg/.tga/.png in turn — the first pk3
 * entry that exists and decodes wins. Generates a full mip chain and an
 * anisotropic-filtering sampler.
 *
 * @return An upload with a null texture if no candidate was found; the
 *         caller falls back to CreateBspWhiteTexture() in that case.
 */
BspTextureUpload LoadBspTextureFromPk3(
    zip_t* archive, SDL_GPUDevice* device, const std::string& texName,
    const std::map<std::string, std::string>& shaderImages);

/// A 1x1 white texture + clamped sampler, for textures missing from the pk3.
BspTextureUpload CreateBspWhiteTexture(SDL_GPUDevice* device);

}  // namespace sdl3cpp::services::impl
