#pragma once

#include "services/interfaces/workflow/rendering/draw_map_texture_config.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief Draws every legacy (non-BSP) mesh in `mapNodes`, one draw call each.
 *
 * Each mesh has its own VB/IB with 16-bit indices; the mesh's texture is
 * whichever mapping's pattern is found in the mesh name (or `defaultTexture`
 * if none match), and its normal is derived from the thinnest axis of its
 * bounding box when present.
 */
void DrawLegacyMapGeometry(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                           WorkflowContext& context,
                           const nlohmann::json& mapNodes,
                           rendering::VertexUniformData vu,
                           const rendering::FragmentUniformData& fu,
                           const DrawMapTextureConfig& config);

}  // namespace sdl3cpp::services::impl
