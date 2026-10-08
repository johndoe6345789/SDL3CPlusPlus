#pragma once

#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Places the arrow `height` metres above `car`, turned to face `target`
/// across the ground.
glm::mat4 BuildSwitchbackArrowModel(const glm::vec3& car,
                                    const glm::vec3& target, float height);

/// The arrow's vertex uniforms for one draw: MVP from `model`, the shared
/// view and projection, and an identity shadow matrix.
rendering::VertexUniformData BuildSwitchbackArrowUniforms(
    const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj,
    const glm::vec3& cameraPos);

}  // namespace sdl3cpp::services::impl
