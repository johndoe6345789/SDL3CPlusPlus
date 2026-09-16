#include "services/interfaces/workflow/graphics/video_recorder_ops.hpp"

#include <string>

namespace sdl3cpp::services::impl {
namespace {

SDL_GPUTexture* CreateTarget(SDL_GPUDevice* device,
                             SDL_GPUTextureFormat format,
                             std::uint32_t width, std::uint32_t height) {
    SDL_GPUTextureCreateInfo info = {};
    info.type                     = SDL_GPU_TEXTURETYPE_2D;
    info.format                   = format;
    info.width                    = width;
    info.height                   = height;
    info.layer_count_or_depth     = 1;
    info.num_levels               = 1;
    info.usage =
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    return SDL_CreateGPUTexture(device, &info);
}

}  // namespace

// SDL's Vulkan swapchain images cannot be read back, so a recorded
// frame is drawn here instead, then blitted to the swapchain.
SDL_GPUTexture* EnsureVideoTarget(VideoRecorder& rec, SDL_Window* window,
                                  std::uint32_t width,
                                  std::uint32_t height) {
    if (rec.target && rec.targetWidth == width &&
        rec.targetHeight == height) {
        return rec.target;
    }
    if (rec.target) SDL_ReleaseGPUTexture(rec.device, rec.target);
    rec.target = nullptr;

    const auto format =
        SDL_GetGPUSwapchainTextureFormat(rec.device, window);
    if (!VideoByteOrder(format, rec.bgra)) {
        if (rec.logger) {
            rec.logger->Error("video.record: swapchain format " +
                              std::to_string(format) + " is not 8-bit");
        }
        rec.done = true;
        return nullptr;
    }
    rec.target       = CreateTarget(rec.device, format, width, height);
    rec.targetWidth  = width;
    rec.targetHeight = height;
    return rec.target;
}

}  // namespace sdl3cpp::services::impl
