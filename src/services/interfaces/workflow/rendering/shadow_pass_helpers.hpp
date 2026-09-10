#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <string>

namespace sdl3cpp::services::impl {

/// The 6 constant face-rotation matrices shared by every shadow-casting
/// box (each face's local +Y becomes that face's outward normal).
struct ShadowFaceRotations {
    glm::mat4 none;
    glm::mat4 down;
    glm::mat4 north;
    glm::mat4 south;
    glm::mat4 east;
    glm::mat4 west;
};

ShadowFaceRotations BuildShadowFaceRotations();

/**
 * @brief Draws all 6 faces of one shadow-casting box body.
 *
 * Skips bodies larger than 15 units on any axis (floor/walls/ceiling
 * aren't shadow casters), exactly as shadow.pass always has. Looks up the
 * body's synced transform under `body_sync_<name>`; a no-op if that key or
 * `pos`/`size`/`rotation` are missing.
 */
void DrawShadowCasterBody(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                          const WorkflowContext& context,
                          const std::string& name, const glm::mat4& lightVP,
                          const ShadowFaceRotations& rotations,
                          uint32_t indexCount);

}  // namespace sdl3cpp::services::impl
