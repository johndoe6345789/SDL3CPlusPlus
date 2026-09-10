#pragma once

#include "services/interfaces/workflow/rendering/grid_draw_config.hpp"
#include "services/interfaces/workflow/rendering/grid_gpu_resources.hpp"

namespace sdl3cpp::services::impl {

/// Acquires the swapchain texture and begins the clear+depth render pass;
/// returns nullptr (after submitting the now-empty command buffer) if the
/// swapchain texture or pass couldn't be acquired.
SDL_GPURenderPass* BeginGridRenderPass(SDL_GPUCommandBuffer* cmd,
                                       const GridGpuResources& gpu,
                                       const GridDrawConfig& cfg);

}  // namespace sdl3cpp::services::impl
