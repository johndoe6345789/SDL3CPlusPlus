#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"

namespace sdl3cpp::services::impl {

void DestroyGpuTextOverlayResources(GpuTextOverlayResources& res) {
    if (res.renderer) {
        SDL_DestroyRenderer(res.renderer);
        res.renderer = nullptr;
    }
    if (res.surface) {
        SDL_DestroySurface(res.surface);
        res.surface = nullptr;
    }
    if (res.device) {
        if (res.sampler) SDL_ReleaseGPUSampler(res.device, res.sampler);
        if (res.vertices) SDL_ReleaseGPUBuffer(res.device, res.vertices);
        if (res.transfer) {
            SDL_ReleaseGPUTransferBuffer(res.device, res.transfer);
        }
        if (res.texture) SDL_ReleaseGPUTexture(res.device, res.texture);
        if (res.pipeline) {
            SDL_ReleaseGPUGraphicsPipeline(res.device, res.pipeline);
        }
    }
    res.sampler  = nullptr;
    res.vertices = nullptr;
    res.transfer = nullptr;
    res.texture  = nullptr;
    res.pipeline = nullptr;
    res.device   = nullptr;
}

}  // namespace sdl3cpp::services::impl
