#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>

namespace sdl3cpp::services::impl {

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

}  // namespace sdl3cpp::services::impl
