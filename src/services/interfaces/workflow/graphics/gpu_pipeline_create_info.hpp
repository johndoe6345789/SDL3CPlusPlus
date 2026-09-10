#pragma once

#include "services/interfaces/workflow/graphics/gpu_pipeline_params.hpp"
#include "services/interfaces/workflow/graphics/gpu_pipeline_vertex_layout.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/**
 * @brief Assembles the full graphics-pipeline create-info from `p` and the
 *        already-resolved shaders/device/window.
 *
 * `layoutOut` and `colorTargetOut` are owned by the caller and must stay
 * alive until the returned info is passed to pipeline creation, since the
 * info's vertex/color-target fields point into them.
 */
SDL_GPUGraphicsPipelineCreateInfo BuildGraphicsPipelineCreateInfo(
    const GpuPipelineCreateParams& p, SDL_GPUShader* vertexShader,
    SDL_GPUShader* fragmentShader, SDL_GPUDevice* device, SDL_Window* window,
    GpuVertexAttributeLayout& layoutOut,
    SDL_GPUColorTargetDescription& colorTargetOut);

}  // namespace sdl3cpp::services::impl
