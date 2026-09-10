#pragma once

#include <SDL3/SDL_gpu.h>
#include <string>

namespace sdl3cpp::services::impl {

/// "front"/"none"/"back" (default) as an SDL_GPUCullMode.
SDL_GPUCullMode ResolveCullMode(const std::string& cullMode);

/// "d24_unorm_s8" or "d32_float" (default) as an SDL_GPUTextureFormat.
SDL_GPUTextureFormat ResolveDepthFormat(const std::string& depthFormat);

/**
 * @brief Resolves a named color target format.
 *
 * "rgba16_float", "r8_unorm", "b8g8r8a8_unorm" resolve directly; the
 * default "swapchain" queries the window's swapchain format, falling back
 * to B8G8R8A8_UNORM if no window is available.
 */
SDL_GPUTextureFormat ResolveColorTargetFormat(const std::string& colorFormat,
                                              SDL_GPUDevice* device,
                                              SDL_Window* window);

/// Enables standard src-alpha/one-minus-src-alpha blending on `target`.
void ApplyAlphaBlendState(SDL_GPUColorTargetDescription& target);

}  // namespace sdl3cpp::services::impl
