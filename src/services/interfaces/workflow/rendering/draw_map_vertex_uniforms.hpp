#pragma once

#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Builds the vertex uniforms shared by every draw call this frame: MVP for
/// an identity model matrix, camera position, and the shadow-pass VP. The
/// normal starts as +Y; legacy (non-BSP) mode overwrites it per mesh.
rendering::VertexUniformData BuildDrawMapVertexUniforms(
    const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos,
    const glm::mat4& shadowVP);

}  // namespace sdl3cpp::services::impl
