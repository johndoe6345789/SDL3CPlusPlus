#include "services/interfaces/workflow/gta5/gta5_indexed_texture.hpp"

#include "services/interfaces/workflow/gta5/gta5_resource_cache.hpp"
#include "services/interfaces/workflow/gta5/gta5_texture_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"

#include <string>

namespace sdl3cpp::services::impl {

const Gta5Texture* GetOrLoadGta5IndexedTexture(Gta5StreamState& state,
                                               std::uint32_t nameHash,
                                               SDL_GPUDevice* device) {
    if (nameHash == 0 || !state.assets || !device) return nullptr;

    // Keyed apart from file paths, which the glTF path still uses.
    const std::string key = "tex:" + std::to_string(nameHash);
    const auto cached = state.textureCache.find(key);
    if (cached != state.textureCache.end()) {
        return cached->second.usable ? &cached->second : nullptr;
    }
    // Inserted first, so a miss is remembered rather than searched again.
    Gta5Texture& entry = state.textureCache[key];

    const auto where = state.assets->textures.find(nameHash);
    if (where == state.assets->textures.end()) return nullptr;
    const auto ytd =
        AcquireGta5Resource(state.resources, *state.assets, where->second);
    if (!ytd) return nullptr;

    const Gta5GpuTexture gpu =
        UploadGta5DictionaryTexture(*ytd, nameHash, device);
    if (!gpu.texture) return nullptr;
    entry.sampler = CreateTextureLoadSampler(device, gpu.texture, gpu.levels);
    if (!entry.sampler) {
        SDL_ReleaseGPUTexture(device, gpu.texture);
        return nullptr;
    }
    entry.texture = gpu.texture;
    entry.usable = true;
    return &entry;
}

}  // namespace sdl3cpp::services::impl
