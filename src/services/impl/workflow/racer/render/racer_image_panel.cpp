#include "services/interfaces/workflow/racer/render/racer_screen_step.hpp"

#include "services/interfaces/workflow/racer/data/racer_asset_io.hpp"
#include "services/interfaces/workflow/racer/world/racer_gpu_upload.hpp"

namespace sdl3cpp::services::impl {

bool CreateRacerImagePanel(SDL_GPUDevice* device,
                           const std::filesystem::path& path,
                           RacerPanel& out) {
    const auto image = ReadRacerImage(path);
    if (!device || !image) return false;
    RacerTexture texture;
    texture.width = image->width;
    texture.height = image->height;
    texture.rgba = image->rgba;
    // Painted art: a smooth, mipmapped sampler suits it better than the
    // pixel-preserving Scale2x used on the game's palette textures.
    const RacerGpuTexture gpu = UploadRacerTexture(device, texture, 1);
    if (!gpu.texture) return false;
    out = RacerPanel{};
    out.device = device;
    out.texture = gpu.texture;
    out.sampler = gpu.sampler;
    out.width = image->width;
    out.height = image->height;
    return true;
}

RacerScreenRect RacerCoverRect(float imageAspect, float screenAspect) {
    RacerScreenRect rect;
    if (screenAspect > imageAspect) {
        const float keep = imageAspect / screenAspect;  // of the height
        rect.v0 = 0.5f * (1.f - keep);
        rect.v1 = 1.f - rect.v0;
    } else {
        const float keep = screenAspect / imageAspect;  // of the width
        rect.u0 = 0.5f * (1.f - keep);
        rect.u1 = 1.f - rect.u0;
    }
    return rect;
}

}  // namespace sdl3cpp::services::impl
