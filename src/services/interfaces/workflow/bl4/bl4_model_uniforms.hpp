#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Matches VertexUniforms in bl4_model.vert.
struct Bl4ModelVertexUniforms {
    glm::mat4 viewProj{1.f};
    glm::mat4 model{1.f};
};

/// Matches FragmentUniforms in bl4_model.frag. Colours are linear.
struct Bl4ModelFragmentUniforms {
    glm::vec4 sunDir{0.f, -1.f, 0.f, 0.f};  // direction light travels
    glm::vec4 sunColor{1.f};
    glm::vec4 ambient{0.4f, 0.45f, 0.55f, 1.f};
};

/// viewProj from render.view_matrix/render.proj_matrix; model is filled
/// in per instance by the draw step.
glm::mat4 BuildBl4ViewProj(const WorkflowContext& context);

/// Sun and ambient from lighting.setup (render.frag_uniforms).
Bl4ModelFragmentUniforms BuildBl4ModelFragmentUniforms(const WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
