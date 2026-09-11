#include "services/interfaces/workflow/rendering/gpu_text_overlay_upload.hpp"

#include <SDL3/SDL_render.h>

#include <cstring>

namespace sdl3cpp::services::impl {

bool UploadGpuTextOverlayText(const GpuTextOverlayResources& res,
                              SDL_GPUCommandBuffer* cmd, const char* text,
                              SDL_Color colour) {
    if (!cmd || !res.surface || !res.renderer || !res.transfer) return false;

    SDL_ClearSurface(res.surface, 0.0f, 0.0f, 0.0f, 0.0f);
    if (text && text[0] != '\0') {
        SDL_SetRenderDrawColor(res.renderer, colour.r, colour.g, colour.b,
                               colour.a);
        SDL_RenderDebugText(res.renderer, 5.0f, 2.0f, text);
    }

    void* mapped = SDL_MapGPUTransferBuffer(res.device, res.transfer, false);
    if (!mapped) return false;
    std::memcpy(
        mapped, res.surface->pixels,
        static_cast<size_t>(kGpuTextOverlayWidth * kGpuTextOverlayHeight * 4));
    SDL_UnmapGPUTransferBuffer(res.device, res.transfer);

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    if (!copy) return false;

    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer = res.transfer;
    src.pixels_per_row = static_cast<Uint32>(kGpuTextOverlayWidth);
    src.rows_per_layer = static_cast<Uint32>(kGpuTextOverlayHeight);

    SDL_GPUTextureRegion dst = {};
    dst.texture = res.texture;
    dst.w = static_cast<Uint32>(kGpuTextOverlayWidth);
    dst.h = static_cast<Uint32>(kGpuTextOverlayHeight);
    dst.d = 1;

    SDL_UploadToGPUTexture(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    return true;
}

}  // namespace sdl3cpp::services::impl
