#pragma once

#include "services/interfaces/workflow/rendering/frame_clear_params.hpp"
#include "services/interfaces/workflow/rendering/frame_swapchain_acquire.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Gets a cached GPU texture from context, (re)creating it if
 * missing or sized for a different swapchain.
 *
 * Reads/writes `textureKey`, `widthKey` and `heightKey` in context.
 * Releases the old texture (if any) before replacing it.
 */
SDL_GPUTexture* GetOrResizeTexture(
    SDL_GPUDevice* device, WorkflowContext& context,
    const std::string& textureKey, const std::string& widthKey,
    const std::string& heightKey, SDL_GPUTextureFormat format,
    SDL_GPUTextureUsageFlags usage, uint32_t width, uint32_t height);

/// Begins a render pass with one color target (cleared to `clear`) and
/// one depth target (cleared to 1.0, not stored).
SDL_GPURenderPass* BeginColorDepthRenderPass(SDL_GPUCommandBuffer* cmd,
                                             SDL_GPUTexture* colorTexture,
                                             const ClearColorParams& clear,
                                             SDL_GPUTexture* depthTexture);

}  // namespace sdl3cpp::services::impl
