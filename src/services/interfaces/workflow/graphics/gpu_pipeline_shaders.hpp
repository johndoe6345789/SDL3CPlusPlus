#pragma once

#include "services/interfaces/workflow/graphics/gpu_pipeline_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// The precompiled vertex/fragment shaders a graphics pipeline is built
/// from.
struct GpuPipelineShaders {
    SDL_GPUShader* vertex   = nullptr;
    SDL_GPUShader* fragment = nullptr;
};

/**
 * @brief Looks up the vertex/fragment shaders `p` names in `context`.
 * @throws std::runtime_error (naming the missing key) if either is absent.
 */
GpuPipelineShaders RequireGpuPipelineShaders(WorkflowContext& context,
                                             const GpuPipelineCreateParams& p);

}  // namespace sdl3cpp::services::impl
