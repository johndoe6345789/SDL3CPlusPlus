#pragma once

#include <glm/glm.hpp>

#include <array>

namespace sdl3cpp::services::impl {

/// The six planes of the view volume, normals pointing inwards.
struct Fs2024Frustum {
    std::array<glm::vec4, 6> planes{};
};

Fs2024Frustum MakeFs2024Frustum(const glm::mat4& viewProj);

/// False only when the box lies wholly outside one of the planes.
bool Fs2024BoxVisible(const Fs2024Frustum& frustum, const glm::vec3& min,
                      const glm::vec3& max);

}  // namespace sdl3cpp::services::impl
