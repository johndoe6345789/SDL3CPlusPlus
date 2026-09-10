#include "services/interfaces/workflow/graphics/gpu_swapchain_capture.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_surface.h>

namespace sdl3cpp::services::impl {
namespace {

/// SDL_GPU texture formats are named by channel order in a 32-bit word;
/// SDL_PixelFormat packed names are read the same way, so the two line
/// up directly. Most Windows/Vulkan swapchains are B8G8R8A8, not
/// R8G8B8A8 -- assuming the latter swaps red and blue in the saved
/// image, so map the real format instead of guessing.
SDL_PixelFormat ToSdlPixelFormat(SDL_GPUTextureFormat format) {
    switch (format) {
    case SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM:
    case SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM_SRGB:
        return SDL_PIXELFORMAT_ARGB8888;
    case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM:
    case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB:
    default:
        return SDL_PIXELFORMAT_ABGR8888;
    }
}

bool SaveDownloadedPixels(void* pixels, uint32_t width, uint32_t height,
                          SDL_GPUTextureFormat format,
                          const std::string& path,
                          const std::shared_ptr<ILogger>& logger) {
    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        static_cast<int>(width), static_cast<int>(height),
        ToSdlPixelFormat(format), pixels, static_cast<int>(width * 4));
    if (!surface) {
        return false;
    }
    const bool saved = SDL_SaveBMP(surface, path.c_str());
    SDL_DestroySurface(surface);
    if (logger) {
        if (saved) {
            logger->Info("gpu.screenshot_capture: GPU screenshot saved to " +
                        path);
        } else {
            logger->Error("gpu.screenshot_capture: failed to save " + path +
                         ": " + SDL_GetError());
        }
    }
    return saved;
}

}  // namespace

bool CaptureGpuSwapchainToBmp(SDL_GPUCommandBuffer* cmd, SDL_GPUDevice* device,
                              SDL_GPUTexture* swapchain, uint32_t width,
                              uint32_t height, SDL_GPUTextureFormat format,
                              const std::string& path,
                              const std::shared_ptr<ILogger>& logger) {
    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    tbci.size                            = width * height * 4;
    auto* staging = SDL_CreateGPUTransferBuffer(device, &tbci);
    if (!staging) {
        return false;
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

    bool saved = false;
    if (void* mapped = SDL_MapGPUTransferBuffer(device, staging, false)) {
        saved =
            SaveDownloadedPixels(mapped, width, height, format, path, logger);
        SDL_UnmapGPUTransferBuffer(device, staging);
    }
    SDL_ReleaseGPUTransferBuffer(device, staging);
    return saved;
}

}  // namespace sdl3cpp::services::impl
