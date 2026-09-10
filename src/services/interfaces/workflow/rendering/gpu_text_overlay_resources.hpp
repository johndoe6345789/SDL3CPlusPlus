#pragma once

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>

namespace sdl3cpp::services::impl {

/// Pixel size of the FPS overlay texture.
inline constexpr int kGpuTextOverlayWidth  = 160;
inline constexpr int kGpuTextOverlayHeight = 12;

/**
 * @brief GPU and CPU resources backing a small text overlay quad.
 *
 * Shared by the `postfx.overlay_fps_*` and `overlay.fps_*` step families:
 * whichever `*_init` step runs owns one instance and publishes a pointer to it
 * for its sibling steps.  Every member is null until
 * CreateGpuTextOverlayResources() reports success.
 */
struct GpuTextOverlayResources {
    SDL_GPUDevice* device             = nullptr;
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    SDL_GPUTexture* texture           = nullptr;
    SDL_GPUTransferBuffer* transfer   = nullptr;
    SDL_GPUBuffer* vertices           = nullptr;
    SDL_GPUSampler* sampler           = nullptr;
    SDL_Surface* surface              = nullptr;
    SDL_Renderer* renderer            = nullptr;
};

/**
 * @brief Creates the overlay pipeline, texture, buffers and text surface.
 *
 * Rolls back every resource it already created if a later stage fails, so the
 * struct is either fully populated or left untouched.
 *
 * @param device        GPU device to allocate from.
 * @param swapchainFmt  Colour target format the overlay blends into.
 * @param out           Populated on success; unmodified on failure.
 * @return Empty string on success, otherwise the reason the overlay is
 *         unavailable (a caller-loggable, non-fatal condition).
 */
const char* CreateGpuTextOverlayResources(SDL_GPUDevice* device,
                                          SDL_GPUTextureFormat swapchainFmt,
                                          GpuTextOverlayResources& out);

/// Releases everything CreateGpuTextOverlayResources() allocated and nulls it.
void DestroyGpuTextOverlayResources(GpuTextOverlayResources& res);

}  // namespace sdl3cpp::services::impl
