#include "services/interfaces/workflow/bl4/bl4_texture_cache.hpp"

#include "services/interfaces/workflow/graphics/texture_gpu_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_image_io.hpp"
#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"

#include <exception>

namespace sdl3cpp::services::impl {

Bl4Texture* AcquireBl4Texture(Bl4TextureCache& cache,
                              const std::string& path,
                              SDL_GPUDevice* device,
                              const std::shared_ptr<ILogger>& logger) {
    if (path.empty() || !device) return nullptr;
    const auto found = cache.find(path);
    if (found != cache.end()) {
        if (!found->second.usable) return nullptr;
        ++found->second.references;
        return &found->second;
    }

    Bl4Texture& entry = cache[path];
    try {
        LoadedTextureImage image = LoadTextureImagePixels(path);
        const UploadedTexture uploaded = UploadTextureImage(device, image);
        entry.texture = uploaded.texture;
        entry.sampler = CreateTextureLoadSampler(
            device, uploaded.texture, uploaded.numLevels, 0.f);
        entry.usable = true;
        entry.references = 1;
    } catch (const std::exception& error) {
        if (logger) {
            logger->Warn("bl4.tiles.load: texture '" + path + "': " +
                         error.what());
        }
        return nullptr;
    }
    return &entry;
}

void ReleaseBl4Texture(Bl4TextureCache& cache, const std::string& path,
                       SDL_GPUDevice* device) {
    const auto found = cache.find(path);
    if (found == cache.end() || !found->second.usable) return;
    if (--found->second.references > 0) return;
    if (device) {
        SDL_ReleaseGPUSampler(device, found->second.sampler);
        SDL_ReleaseGPUTexture(device, found->second.texture);
    }
    cache.erase(found);
}

}  // namespace sdl3cpp::services::impl
