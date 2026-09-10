#pragma once

#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief Reads `camera.state.view`/`.projection` (16-float arrays, a no-op
 * per matrix if missing or the wrong size), computes the camera's world
 * position as the inverse view matrix's translation column, and stores all
 * three under `render.view_matrix`/`render.proj_matrix`/`render.camera_pos`.
 *
 * @return the computed camera position, for render.prepare's log line.
 */
glm::vec3 PrepareRenderCameraState(WorkflowContext& context);

/// Reads `shadow.state.light_vp` (a 16-float array, defaulting to identity
/// if missing or the wrong size) and stores it under `render.shadow_vp`.
void PrepareRenderShadowState(WorkflowContext& context);

/**
 * @brief Builds the fragment (PBR lighting) uniforms from
 * `lighting.directional` and stores them under `render.frag_uniforms`.
 *
 * Defaults (used for any field `lighting.directional` doesn't set): light
 * pointing straight down, white light and 0.2 grey ambient, roughness 0.8,
 * metallic 0.
 */
void PrepareRenderLightingState(WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
