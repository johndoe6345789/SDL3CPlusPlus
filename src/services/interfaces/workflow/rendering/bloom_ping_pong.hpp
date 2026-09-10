#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>

namespace sdl3cpp::services::impl {

/// Half-res RGBA16F ping/pong textures shared by the bloom passes.
struct BloomPingPongTextures {
    SDL_GPUTexture* ping = nullptr;
    SDL_GPUTexture* pong = nullptr;
};

/**
 * @brief Creates (or recreates, on a size change) the bloom ping/pong
 *        textures and stores them in context.
 *
 * Reuses the existing pair from "postfx_bloom_ping_texture"/
 * "_pong_texture" when their recorded size ("postfx_bloom_ping_width"/
 * "_height") already matches `halfW`/`halfH`; otherwise releases the old
 * pair and creates a new one.
 */
BloomPingPongTextures EnsureBloomPingPongTextures(SDL_GPUDevice* device,
                                                  WorkflowContext& context,
                                                  uint32_t halfW,
                                                  uint32_t halfH);

/**
 * @brief Draws the bright-pixel extraction pass into `target`.
 *
 * Uses a fixed threshold=1.0/soft_knee=0.5 (luminance above the
 * threshold triggers bloom, with a smooth falloff over the knee width).
 * @return false if the render pass could not be opened.
 */
bool DrawBloomExtractPass(SDL_GPUCommandBuffer* cmd,
                          SDL_GPUGraphicsPipeline* pipeline,
                          SDL_GPUTexture* hdrTex, SDL_GPUSampler* sampler,
                          SDL_GPUTexture* target);

}  // namespace sdl3cpp::services::impl
