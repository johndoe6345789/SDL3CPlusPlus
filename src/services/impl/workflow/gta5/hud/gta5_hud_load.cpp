#include "services/interfaces/workflow/gta5/hud/gta5_hud.hpp"

#include "services/interfaces/workflow/rendering/gpu_text_overlay_pipeline_factory.hpp"
#include "services/interfaces/workflow/rendering/gpu_text_overlay_shader_loader.hpp"

namespace sdl3cpp::services::impl {

bool LoadGta5Hud(Gta5Hud& hud, SDL_GPUDevice* device,
                 SDL_GPUTextureFormat format, Gta5UploadBatch& uploads,
                 const std::shared_ptr<ILogger>& logger) {
    hud.tried = true;
    // The map overlay's shaders and pipeline: textured quads, blended.
    SDL_GPUShader* vertex = nullptr;
    SDL_GPUShader* fragment = nullptr;
    if (*LoadOverlayShaderPair(device, vertex, fragment) == '\0') {
        hud.overlay.pipeline =
            CreateOverlayPipeline(device, format, vertex, fragment);
        SDL_ReleaseGPUShader(device, vertex);
        SDL_ReleaseGPUShader(device, fragment);
    }
    // No categories: the atlas is just the lettering.
    if (!hud.overlay.pipeline || !CreateGta5MapGpu(hud.overlay, device) ||
        !CreateGta5MapMarkers(hud.overlay, device, uploads) ||
        !CreateGta5MapAtlas(hud.overlay, device, uploads) ||
        !CreateGta5HudArt(hud, device, uploads)) {
        if (logger) logger->Warn("gta5.hud: could not create the display");
        return false;
    }
    uploads.Flush(device);
    hud.ready = true;
    if (logger) logger->Info("gta5.hud: ready");
    return true;
}

}  // namespace sdl3cpp::services::impl
