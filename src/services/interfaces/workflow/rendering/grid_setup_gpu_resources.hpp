#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// Throws std::runtime_error naming whichever GPU resource is missing:
/// device/window, pipeline, or vertex/index buffers.
void ValidateGridSetupGpuResources(const WorkflowContext& context);

/// Creates a D32_FLOAT depth-stencil target sized to the window. Throws
/// std::runtime_error on failure.
SDL_GPUTexture* CreateGridDepthTexture(SDL_GPUDevice* device, int width,
                                       int height);

}  // namespace sdl3cpp::services::impl
