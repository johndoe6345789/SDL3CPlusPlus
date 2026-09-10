#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Draws one physics body as a unit cube, if it's visible.
 *
 * Reads `physics_visual_<name>` for visibility/spin/scale and
 * `physics_body_<name>` for the Bullet transform; builds the model matrix
 * (translate, then the body's rotation, then an optional time-based spin,
 * then scale — in that order) and issues one indexed draw call of 36
 * indices (a unit cube). A no-op (returns false) if the body is marked
 * invisible or `physics_body_<name>` isn't in the context.
 *
 * @return true if a draw call was issued.
 */
bool DrawOnePhysicsBody(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                        const WorkflowContext& context, const std::string& name,
                        const glm::mat4& viewProj, float time);

}  // namespace sdl3cpp::services::impl
