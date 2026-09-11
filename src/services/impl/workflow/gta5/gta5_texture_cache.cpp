#include "services/interfaces/workflow/gta5/gta5_texture_cache.hpp"

#include "services/interfaces/workflow/graphics/texture_gpu_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_image_io.hpp"
#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"

#include <exception>

namespace sdl3cpp::services::impl {

const Gta5Texture* GetOrLoadGta5Texture(
    Gta5TextureCache& cache, const std::string& path, SDL_GPUDevice* device,
    const std::shared_ptr<ILogger>& logger) {
    if (path.empty() || !device) return nullptr;

    const auto cached = cache.find(path);
    if (cached != cache.end()) {
        return cached->second.usable ? &cached->second : nullptr;
    }

    // Inserted before loading so a failure is remembered as unusable,
    // rather than retried for every archetype that names it.
    Gta5Texture& entry = cache[path];
    entry.usable = false;
    try {
        LoadedTextureImage image = LoadTextureImagePixels(path);
        const UploadedTexture uploaded = UploadTextureImage(device, image);
        entry.texture = uploaded.texture;
        entry.sampler = CreateTextureLoadSampler(device, uploaded.texture,
                                                 uploaded.numLevels, 0.f);
        entry.usable = true;
    } catch (const std::exception& ex) {
        if (logger) {
            logger->Warn("gta5.tiles.load: texture '" + path + "': " +
                         ex.what());
        }
        return nullptr;
    }
    return &entry;
}

void ClearGta5TextureCache(Gta5TextureCache& cache, SDL_GPUDevice* device) {
    if (device) {
        for (auto& entry : cache) {
            if (!entry.second.usable) continue;
            SDL_ReleaseGPUSampler(device, entry.second.sampler);
            SDL_ReleaseGPUTexture(device, entry.second.texture);
        }
    }
    cache.clear();
}

}  // namespace sdl3cpp::services::impl
