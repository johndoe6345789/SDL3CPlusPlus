#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// Draws `indexCount` indices from `vertexBuffer`/`indexBuffer` as a
/// screen-space overlay (identity MVP) using the textured pipeline
/// and the palette texture. Does nothing if either texture is absent.
void DrawStuntsDashboardMesh(WorkflowContext& context,
                            SDL_GPURenderPass* pass,
                            SDL_GPUCommandBuffer* cmd,
                            SDL_GPUGraphicsPipeline* pipeline,
                            SDL_GPUBuffer* vertexBuffer,
                            SDL_GPUBuffer* indexBuffer,
                            std::uint32_t indexCount);

}  // namespace sdl3cpp::services::impl
