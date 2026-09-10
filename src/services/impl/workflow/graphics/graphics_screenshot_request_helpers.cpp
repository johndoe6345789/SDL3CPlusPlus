#include "services/interfaces/workflow/graphics/graphics_screenshot_request_helpers.hpp"

#include <cstdlib>
#include <filesystem>

namespace sdl3cpp::services::impl {

std::string ResolveScreenshotOutputPath(const std::string& rawPath) {
    std::string resolved = rawPath;
    if (!resolved.empty() && resolved[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) {
            resolved = std::string(home) + resolved.substr(1);
        }
    }

    std::filesystem::path path(resolved);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    return resolved;
}

std::string ToBmpPath(const std::string& path) {
    if (path.size() > 4 && path.substr(path.size() - 4) == ".png") {
        return path.substr(0, path.size() - 4) + ".bmp";
    }
    return path;
}

StagingCapture BlitSwapchainToStagingTexture(SDL_GPUDevice* device,
                                             SDL_Window* window) {
    int w = 0, h = 0;
    SDL_GetWindowSize(window, &w, &h);
    if (w <= 0 || h <= 0) {
        return {};
    }

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);

    SDL_GPUTexture* swapchainTex = nullptr;
    Uint32 sw = 0, sh = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchainTex, &sw,
                                               &sh) ||
        !swapchainTex) {
        SDL_CancelGPUCommandBuffer(cmd);
        return {};
    }

    const SDL_GPUTextureFormat format =
        SDL_GetGPUSwapchainTextureFormat(device, window);
    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type                     = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format                   = format;
    texInfo.width                    = sw;
    texInfo.height                   = sh;
    texInfo.layer_count_or_depth     = 1;
    texInfo.num_levels               = 1;
    texInfo.usage =
        SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;

    SDL_GPUTexture* stagingTex = SDL_CreateGPUTexture(device, &texInfo);
    if (!stagingTex) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return {};
    }

    SDL_GPUBlitInfo blit     = {};
    blit.source.texture      = swapchainTex;
    blit.source.w            = sw;
    blit.source.h            = sh;
    blit.destination.texture = stagingTex;
    blit.destination.w       = sw;
    blit.destination.h       = sh;
    blit.load_op             = SDL_GPU_LOADOP_DONT_CARE;
    blit.filter              = SDL_GPU_FILTER_LINEAR;

    SDL_BlitGPUTexture(cmd, &blit);
    SDL_SubmitGPUCommandBuffer(cmd);

    return StagingCapture{stagingTex, sw, sh};
}

}  // namespace sdl3cpp::services::impl
