#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <cstdint>

namespace sdl3cpp::services::impl {

/// Halton sequence for sub-pixel jitter (low discrepancy, covers a
/// pixel well).
float Halton(int index, int base);

/**
 * @brief Jitters `render.proj_matrix` in context for the *next* frame.
 *
 * The current frame was rendered with the jitter applied last frame, so
 * this must run after the frame is drawn but before the next frame's
 * projection matrix is consumed.
 */
void ApplyTaaProjectionJitter(WorkflowContext& context, int frameIdx,
                              uint32_t width, uint32_t height);

/// Lazily creates and caches postfx.taa's resolve pipeline in context.
/// Returns nullptr if the required shaders aren't compiled yet.
SDL_GPUGraphicsPipeline* GetOrCreateTaaPipeline(SDL_GPUDevice* device,
                                                WorkflowContext& context);

/// The ping-pong pair selected for this frame's TAA resolve: read the
/// previous frame's result, write this frame's.
struct TaaHistoryTextures {
    SDL_GPUTexture* read  = nullptr;
    SDL_GPUTexture* write = nullptr;
};

/// Lazily (re)creates the ping-pong history textures in context if they
/// are missing or the wrong size, then flips and returns which is read
/// vs. written this frame.
TaaHistoryTextures GetOrCreateTaaHistoryTextures(SDL_GPUDevice* device,
                                                 WorkflowContext& context,
                                                 uint32_t width,
                                                 uint32_t height);

/// Draws the fullscreen TAA resolve pass: blends `hdrTex` with
/// `history.read` into `history.write`.
void DrawTaaResolvePass(SDL_GPUCommandBuffer* cmd,
                        SDL_GPUGraphicsPipeline* pipeline,
                        SDL_GPUTexture* hdrTex,
                        const TaaHistoryTextures& history,
                        SDL_GPUSampler* sampler, float blendFactor,
                        uint32_t width, uint32_t height,
                        double frameCount);

}  // namespace sdl3cpp::services::impl
