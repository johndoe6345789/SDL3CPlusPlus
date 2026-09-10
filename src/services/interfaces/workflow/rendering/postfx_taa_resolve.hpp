#pragma once

#include "services/interfaces/workflow/rendering/postfx_taa_history.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>

namespace sdl3cpp::services::impl {

/// Draws the fullscreen TAA resolve pass: blends `hdrTex` with
/// `history.read` into `history.write`.
void DrawTaaResolvePass(SDL_GPUCommandBuffer* cmd,
                        SDL_GPUGraphicsPipeline* pipeline,
                        SDL_GPUTexture* hdrTex,
                        const TaaHistoryTextures& history,
                        SDL_GPUSampler* sampler, float blendFactor,
                        uint32_t width, uint32_t height, double frameCount);

}  // namespace sdl3cpp::services::impl
