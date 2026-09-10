#pragma once

#include "services/interfaces/workflow/rendering/shadow_face_rotations.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

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
