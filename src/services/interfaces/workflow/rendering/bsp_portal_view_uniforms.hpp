#pragma once

#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Builds the vertex/fragment uniforms for rendering the scene as seen from
/// `dest` looking along the player's current yaw/pitch (a 90-degree FOV
/// perspective, matching bsp.portal_view's original teleporter-preview look).
void BuildPortalViewUniforms(const WorkflowContext& context,
                             const glm::vec3& dest,
                             rendering::VertexUniformData& vu,
                             rendering::FragmentUniformData& fu);

}  // namespace sdl3cpp::services::impl
