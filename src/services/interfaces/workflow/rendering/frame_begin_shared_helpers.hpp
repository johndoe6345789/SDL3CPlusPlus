#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <string>

namespace sdl3cpp::services::impl {

/// frame.gpu.begin/frame.gpu.begin_offscreen's shared clear color, 0..1
/// per channel.
struct ClearColorParams {
    float r = 0.1f, g = 0.1f, b = 0.15f;
};

ClearColorParams ReadClearColorParams(const WorkflowStepDefinition& step);

/// The swapchain image acquired for this frame, plus the command buffer
/// it was acquired on.
struct AcquiredSwapchain {
    SDL_GPUCommandBuffer* cmd = nullptr;
    SDL_GPUTexture* texture   = nullptr;
    uint32_t width = 0, height = 0;
    bool ok = false;
};

/**
 * @brief Acquires a command buffer and this frame's swapchain texture.
 *
 * On failure (no command buffer, or the swapchain isn't ready), submits
 * whatever command buffer was acquired and returns ok=false; the caller
 * should set frame_skip and return without touching `cmd` further.
 */
AcquiredSwapchain AcquireSwapchainForFrame(SDL_GPUDevice* device,
                                           SDL_Window* window);

/**
 * @brief Gets a cached GPU texture from context, (re)creating it if
 * missing or sized for a different swapchain.
 *
 * Reads/writes `textureKey`, `widthKey` and `heightKey` in context.
 * Releases the old texture (if any) before replacing it.
 */
SDL_GPUTexture* GetOrResizeTexture(SDL_GPUDevice* device,
                                   WorkflowContext& context,
                                   const std::string& textureKey,
                                   const std::string& widthKey,
                                   const std::string& heightKey,
                                   SDL_GPUTextureFormat format,
                                   SDL_GPUTextureUsageFlags usage,
                                   uint32_t width, uint32_t height);

/// Begins a render pass with one color target (cleared to `clear`) and
/// one depth target (cleared to 1.0, not stored).
SDL_GPURenderPass* BeginColorDepthRenderPass(SDL_GPUCommandBuffer* cmd,
                                             SDL_GPUTexture* colorTexture,
                                             const ClearColorParams& clear,
                                             SDL_GPUTexture* depthTexture);

}  // namespace sdl3cpp::services::impl
