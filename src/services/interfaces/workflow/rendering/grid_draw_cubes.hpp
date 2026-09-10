#pragma once

#include "services/interfaces/workflow/rendering/grid_draw_config.hpp"
#include "services/interfaces/workflow/rendering/grid_gpu_resources.hpp"

#include <cstdint>

namespace sdl3cpp::services::impl {

/// Runs one render pass over the cube grid: acquires a command buffer
/// and swapchain texture, clears, binds buffers, and issues one
/// indexed draw call per cube with its own MVP uniform. Returns the
/// number of draw calls actually submitted (0 if the swapchain/pass
/// could not be acquired).
uint32_t DrawGridCubes(const GridGpuResources& gpu, const GridDrawConfig& cfg,
                       const glm::mat4& view, const glm::mat4& proj,
                       float time);

}  // namespace sdl3cpp::services::impl
