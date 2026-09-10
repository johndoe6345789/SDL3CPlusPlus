#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// Lazily creates and caches postfx.taa's resolve pipeline in context.
/// Returns nullptr if the required shaders aren't compiled yet.
SDL_GPUGraphicsPipeline* GetOrCreateTaaPipeline(SDL_GPUDevice* device,
                                                WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
