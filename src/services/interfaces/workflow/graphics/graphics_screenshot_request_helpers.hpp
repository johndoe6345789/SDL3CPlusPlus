#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <string>

namespace sdl3cpp::services::impl {

/// Expands a leading `~` to $HOME and creates the path's parent
/// directory, so the caller can save straight to the result.
std::string ResolveScreenshotOutputPath(const std::string& rawPath);

/// SDL_SaveBMP requires a .bmp extension; graphics.screenshot.request
/// accepts a .png path (its usual caller-facing name) and renames it.
std::string ToBmpPath(const std::string& path);

/// The staging texture produced by BlitSwapchainToStagingTexture, sized
/// to the current swapchain and already blitted (its GPU commands have
/// been submitted). SDL_GPU_TEXTUREUSAGE_SAMPLER |
/// SDL_GPU_TEXTUREUSAGE_COLOR_TARGET, format matches the swapchain.
struct StagingCapture {
    SDL_GPUTexture* texture = nullptr;
    uint32_t width          = 0;
    uint32_t height         = 0;
};

/**
 * @brief Acquires the current swapchain image and blits it into a new
 * staging texture the caller can download from.
 *
 * Returns a StagingCapture with a null texture if the window has no
 * size yet, the swapchain isn't ready this frame, or the staging
 * texture could not be created -- none of which are errors worth
 * throwing over (e.g. a minimized window, or a request made before the
 * first frame).
 */
StagingCapture BlitSwapchainToStagingTexture(SDL_GPUDevice* device,
                                             SDL_Window* window);

}  // namespace sdl3cpp::services::impl
