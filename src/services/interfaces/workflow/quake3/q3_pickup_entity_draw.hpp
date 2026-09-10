#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Draws one pickup billboard quad, assuming its vertex/index
 *        buffers are already bound on `pass`.
 *
 * Skips drawing (returning silently) if `classname`'s texture isn't
 * cached in `context` yet.
 *
 * @param drawIndex 1-based draw count, used to offset the bob animation
 *                  phase per entity.
 * @param pos World-space anchor position; the vertical bob offset is
 *            applied internally.
 */
void DrawSinglePickup(const std::string& classname, glm::vec3 pos,
                      int drawIndex, const glm::vec3& camRight,
                      const glm::vec3& camUp, const glm::mat4& view,
                      const glm::mat4& proj, const glm::vec3& camPos,
                      const glm::mat4& shadowVP, float time,
                      SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                      WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
