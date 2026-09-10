#include "services/interfaces/workflow/graphics/gpu_swapchain_capture.hpp"

#include <SDL3/SDL_surface.h>

namespace sdl3cpp::services::impl {
namespace {

void SaveDownloadedPixels(void* pixels, uint32_t width, uint32_t height,
                          const std::string& path,
                          const std::shared_ptr<ILogger>& logger) {
    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        static_cast<int>(width), static_cast<int>(height),
        SDL_PIXELFORMAT_ABGR8888, pixels, static_cast<int>(width * 4));
    if (!surface) {
        return;
    }
    SDL_SaveBMP(surface, path.c_str());
    SDL_DestroySurface(surface);
    if (logger) {
        logger->Info("gpu.screenshot_capture: GPU screenshot saved to " + path);
    }
}

}  // namespace

void CaptureGpuSwapchainToBmp(SDL_GPUCommandBuffer* cmd, SDL_GPUDevice* device,
                              SDL_GPUTexture* swapchain, uint32_t width,
                              uint32_t height, const std::string& path,
                              const std::shared_ptr<ILogger>& logger) {
    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    tbci.size                            = width * height * 4;
    auto* staging = SDL_CreateGPUTransferBuffer(device, &tbci);
    if (!staging) {
        return;
    }

    if (SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd)) {
        SDL_GPUTextureRegion src = {};
        src.texture              = swapchain;
        src.w                    = width;
        src.h                    = height;
        src.d                    = 1;

        SDL_GPUTextureTransferInfo dst = {};
        dst.transfer_buffer            = staging;
        dst.pixels_per_row             = width;
        dst.rows_per_layer             = height;

        SDL_DownloadFromGPUTexture(copy, &src, &dst);
        SDL_EndGPUCopyPass(copy);
    }

    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_WaitForGPUIdle(device);

    if (void* mapped = SDL_MapGPUTransferBuffer(device, staging, false)) {
        SaveDownloadedPixels(mapped, width, height, path, logger);
        SDL_UnmapGPUTransferBuffer(device, staging);
    }
    SDL_ReleaseGPUTransferBuffer(device, staging);
}

}  // namespace sdl3cpp::services::impl
