#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/// Parameters for one `draw.textured_box` call, already resolved from the
/// step's JSON parameters.
struct DrawTexturedBoxParams {
    glm::vec3 pos{0.0f};
    glm::vec3 size{1.0f};
    float uvDensity     = 1.0f;
    float roughness     = 0.8f;
    float metallic      = 0.0f;
    std::string texture = "walls_texture";
    std::string body;
};

/**
 * @brief Runs the full `draw.textured_box` draw for one call.
 *
 * Looks up the unit-plane mesh, `params.texture`'s GPU texture/sampler, the
 * optional shadow map, camera/lighting state, and (when `params.body` is
 * set) that body's synced transform, then draws all 6 faces. Logs a
 * warning and returns without drawing if a required resource — the
 * render pass/command buffer/pipeline, the unit plane, or the texture —
 * is missing from `context`.
 */
void DrawTexturedBox(WorkflowContext& context, ILogger* logger,
                     const DrawTexturedBoxParams& params);

}  // namespace sdl3cpp::services::impl
