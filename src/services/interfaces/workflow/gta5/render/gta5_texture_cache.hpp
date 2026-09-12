#pragma once

#include "services/interfaces/i_logger.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/// One texture on the GPU, shared by every submesh that names it.
struct Gta5Texture {
    SDL_GPUTexture* texture{nullptr};
    SDL_GPUSampler* sampler{nullptr};
    std::uint32_t width{0};  // F3 reports these: a small one is a blur
    std::uint32_t height{0};
    /// False once loading has failed, so a missing file costs one
    /// attempt rather than one per archetype.
    bool usable{false};
};

using Gta5TextureCache = std::unordered_map<std::string, Gta5Texture>;

/// Fetch a texture by file path, loading it on first use.
///
/// Returns nullptr when the file is missing or cannot be decoded. The
/// cache outlives individual archetypes deliberately: one texture is
/// typically shared across a whole district.
const Gta5Texture* GetOrLoadGta5Texture(
    Gta5TextureCache& cache, const std::string& path, SDL_GPUDevice* device,
    const std::shared_ptr<ILogger>& logger);

/// Release every cached texture.
void ClearGta5TextureCache(Gta5TextureCache& cache, SDL_GPUDevice* device);

}  // namespace sdl3cpp::services::impl
