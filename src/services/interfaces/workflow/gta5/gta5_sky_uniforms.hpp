#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Matches the SkyUniforms block in gta5_sky.frag.
struct Gta5SkyUniforms {
    glm::mat4 invViewProj{1.f};
    glm::vec4 cameraPos{0.f};
    glm::vec4 sunDir{0.f, -1.f, 0.f, 0.f};
    glm::vec4 horizon{0.55f, 0.60f, 0.70f, 0.f};
    glm::vec4 zenith{0.18f, 0.34f, 0.62f, 1.f};
};

/// Build them from the frame's camera and light.
Gta5SkyUniforms BuildGta5SkyUniforms(const WorkflowStepDefinition& step,
                                     const WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
