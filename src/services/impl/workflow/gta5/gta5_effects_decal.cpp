#include "services/interfaces/workflow/gta5/gta5_effects.hpp"

#include "services/interfaces/workflow/gta5/gta5_texture_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"

namespace sdl3cpp::services::impl {

bool LoadGta5EffectDecals(Gta5Effects& effects, SDL_GPUDevice* device,
                          Gta5UploadBatch& uploads,
                          const std::string& file) {
    if (file.empty()) return false;
    Gta5Resource ytd;
    if (!LoadGta5Resource(file, ytd)) return false;
    // The bullet marks: holes, cracks and the smears around them.
    const Gta5TextureBlob blob =
        ReadGta5DictionaryTexture(ytd, Gta5Hash("fxdecal_bullet_bangs"));
    const Gta5GpuTexture gpu = UploadGta5TextureBlob(blob, device, uploads);
    effects.decals = gpu.texture;
    return effects.decals != nullptr;
}

}  // namespace sdl3cpp::services::impl
