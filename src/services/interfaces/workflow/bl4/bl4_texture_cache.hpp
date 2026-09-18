#pragma once

#include "services/interfaces/i_logger.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/// One base-colour map bl4x baked (textures/<name>.tga), shared by every
/// submesh whose .mtl names it. Ref-counted like Bl4Geometry, so evicting
/// the last tile that uses a texture frees it.
struct Bl4Texture {
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    int references = 0;
    /// False once loading failed: the entry stays, so a missing file
    /// costs one attempt rather than one per archetype.
    bool usable = false;
};

using Bl4TextureCache = std::unordered_map<std::string, Bl4Texture>;

/// Takes a reference to `path`'s texture, loading it on first use.
/// nullptr when the path is empty or the image can't be loaded.
Bl4Texture* AcquireBl4Texture(Bl4TextureCache& cache,
                              const std::string& path,
                              SDL_GPUDevice* device,
                              const std::shared_ptr<ILogger>& logger);

/// Drops one reference taken by AcquireBl4Texture; the GPU texture and
/// sampler are released with the last one.
void ReleaseBl4Texture(Bl4TextureCache& cache, const std::string& path,
                       SDL_GPUDevice* device);

}  // namespace sdl3cpp::services::impl
