#include "services/interfaces/workflow/gta5/gta5_indexed_texture.hpp"

#include "services/interfaces/workflow/gta5/gta5_resource_cache.hpp"
#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"

namespace sdl3cpp::services::impl {

std::string Gta5TextureKey(std::uint32_t nameHash) {
    return "tex:" + std::to_string(nameHash);
}

const Gta5Texture* InstallGta5TextureBlob(Gta5StreamState& state,
                                          const Gta5TextureBlob& blob,
                                          SDL_GPUDevice* device) {
    Gta5Texture& entry = state.textureCache[Gta5TextureKey(blob.hash)];
    if (entry.usable) return &entry;
    const Gta5GpuTexture gpu = UploadGta5TextureBlob(blob, device);
    if (!gpu.texture) return nullptr;  // cached unusable: settled as missing
    entry.sampler = CreateTextureLoadSampler(device, gpu.texture, gpu.levels);
    if (!entry.sampler) {
        SDL_ReleaseGPUTexture(device, gpu.texture);
        return nullptr;
    }
    entry.texture = gpu.texture;
    entry.usable = true;
    return &entry;
}

const Gta5Texture* GetOrLoadGta5IndexedTexture(Gta5StreamState& state,
                                               std::uint32_t nameHash,
                                               SDL_GPUDevice* device) {
    if (nameHash == 0 || !state.assets || !device) return nullptr;
    const auto cached = state.textureCache.find(Gta5TextureKey(nameHash));
    if (cached != state.textureCache.end()) {
        return cached->second.usable ? &cached->second : nullptr;
    }
    Gta5TextureBlob blob;
    blob.hash = nameHash;
    const auto where = state.assets->textures.find(nameHash);
    if (where != state.assets->textures.end()) {
        if (const auto ytd = AcquireGta5Resource(state.resources, *state.assets,
                                                 where->second)) {
            blob = ReadGta5DictionaryTexture(*ytd, nameHash);
        }
    }
    return InstallGta5TextureBlob(state, blob, device);
}

}  // namespace sdl3cpp::services::impl
