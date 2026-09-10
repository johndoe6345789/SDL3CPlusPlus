#pragma once

#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Draws every BSP texture group in `mapNodes` with one shared VB/IB.
 *
 * Binds the single vertex/index buffer once, then for each node binds that
 * group's albedo (falling back to `defaultTexture`) plus shadow/lightmap/
 * portal-destination samplers, and issues one indexed draw call.
 */
void DrawBspMapGeometry(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                        WorkflowContext& context,
                        const nlohmann::json& mapNodes,
                        const rendering::FragmentUniformData& fu,
                        const rendering::VertexUniformData& vu,
                        const std::string& defaultTexture);

}  // namespace sdl3cpp::services::impl
