#pragma once

#include "services/interfaces/workflow/quake3/q3_pickup_quad_buffers.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief Draws a billboard quad for every uncollected pickup entity in
 *        `entities`, textured by pickup category.
 *
 * Skips entities that aren't a recognized pickup classname, are already
 * marked collected in `collected`, or lack a valid "position". Caps at 96
 * draws per call. `time` drives the vertical bob animation.
 */
void DrawPickupEntities(const nlohmann::json& entities,
                        const nlohmann::json& collected, const glm::mat4& view,
                        const glm::mat4& proj, const glm::vec3& camPos,
                        const glm::mat4& shadowVP, float time,
                        SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                        const PickupQuadBuffers& buffers,
                        WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
