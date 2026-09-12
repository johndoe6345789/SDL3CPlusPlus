#include "services/interfaces/workflow/gta5/gta5_weapon_dress.hpp"

#include "services/interfaces/workflow/gta5/gta5_texture_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"

#include <algorithm>
#include <string>

namespace sdl3cpp::services::impl {

/// A weapon keeps its textures in its own .ytd beside the model, which
/// the map's index never scans. Read them there and hand them to the
/// parts that named them, so the gun is not left in the placeholder.
void DressGta5Weapon(Gta5StreamState& state, SDL_GPUDevice* device,
                     const std::string& dir, const std::string& model,
                     const Gta5MeshData& mesh, Gta5Geometry& geometry,
                     const std::shared_ptr<ILogger>& logger) {
    Gta5Resource ytd;
    if (!LoadGta5Resource(dir + "/" + model + ".ytd", ytd)) return;
    if (!state.textureSampler) {
        state.textureSampler = CreateTextureLoadSampler(device, nullptr, 16,
                                                        0.f);
    }
    std::size_t dressed = 0;
    const std::size_t parts =
        std::min(mesh.parts.size(), geometry.subMeshes.size());
    for (std::size_t i = 0; i < parts; ++i) {
        const Gta5TextureBlob blob =
            ReadGta5DictionaryTexture(ytd, mesh.parts[i].textureHash);
        const Gta5GpuTexture gpu =
            UploadGta5TextureBlob(blob, device, state.uploads);
        if (!gpu.texture || !state.textureSampler) continue;
        geometry.subMeshes[i].texture = gpu.texture;
        geometry.subMeshes[i].sampler = state.textureSampler;
        geometry.subMeshes[i].textureSize = {blob.width, blob.height};
        ++dressed;
    }
    if (logger) {
        logger->Info("gta5.weapon: " + model + " wears " +
                     std::to_string(dressed) + " of " +
                     std::to_string(parts));
    }
}

}  // namespace sdl3cpp::services::impl
