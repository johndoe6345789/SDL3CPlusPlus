#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// The four clear-color channels read from the `clear_color` input.
struct FrameClearColor {
    float r;
    float g;
    float b;
    float a;
};

/**
 * @brief Validates and extracts `clearColorJson` as [r,g,b,a].
 * @throws std::runtime_error if it is not a 4-element array.
 */
FrameClearColor ParseFrameClearColorOrThrow(
    const nlohmann::json* clearColorJson);

/**
 * @brief Builds the `frame_id` output value: `{frame_id, skipped}` when
 * skipped, else `{frame_id, clear_color, skipped, timestamp}`.
 */
nlohmann::json BuildFrameBeginOutput(uint32_t frameId, bool skipped,
    const nlohmann::json& clearColorJson);

/**
 * @brief Acquires the GPU command buffer for a new frame.
 * @throws std::runtime_error (naming SDL_GetError()) on failure.
 */
SDL_GPUCommandBuffer* AcquireFrameCommandBufferOrThrow(SDL_GPUDevice* device);

/// Swapchain texture plus its current size, or a null texture if the
/// window is minimized/not visible and the frame should be skipped.
struct SwapchainAcquireResult {
    SDL_GPUTexture* texture;
    Uint32 width;
    Uint32 height;
};

/**
 * @brief Waits for and acquires the swapchain texture for `cmd`.
 * @throws std::runtime_error (naming SDL_GetError(), cancelling `cmd`
 * first) if the underlying SDL call fails outright.
 */
SwapchainAcquireResult AcquireSwapchainTextureOrThrow(
    SDL_GPUCommandBuffer* cmd, SDL_Window* window);

/**
 * @brief Returns `existing` if non-null, otherwise creates a
 * D32_FLOAT depth texture sized `width`x`height`.
 * @throws std::runtime_error (naming SDL_GetError()) on create failure.
 */
SDL_GPUTexture* GetOrCreateFrameDepthTexture(
    SDL_GPUDevice* device, SDL_GPUTexture* existing, Uint32 width,
    Uint32 height);

/**
 * @brief Begins the frame's render pass, clearing `colorTarget` to
 * (r,g,b,a) and `depthTarget` to 1.0.
 * @throws std::runtime_error (naming SDL_GetError()) on failure.
 */
SDL_GPURenderPass* BeginFrameRenderPassOrThrow(
    SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* colorTarget,
    SDL_GPUTexture* depthTarget, float r, float g, float b, float a);

}  // namespace sdl3cpp::services::impl
