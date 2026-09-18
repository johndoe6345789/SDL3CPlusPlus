#pragma once

#include "services/interfaces/workflow/bl4/bl4_geometry.hpp"

#include <glm/glm.hpp>

#include <array>

namespace sdl3cpp::services::impl {

/// The six planes of the view volume, normals pointing inwards.
struct Bl4Frustum {
    std::array<glm::vec4, 6> planes{};
};

Bl4Frustum MakeBl4Frustum(const glm::mat4& viewProj);

/// Whether an instance's world bounding sphere reaches into the view and
/// is big enough there to matter: `sizeRatio` culls one whose radius is
/// below that fraction of its distance from the camera (BL4's kit is
/// full of bolts and cables that cover no pixel at 200 m, and each still
/// costs a draw call). An instance with no bounds is always drawn.
bool Bl4InstanceVisible(const Bl4Frustum& frustum, const Bl4Instance& instance,
                        const glm::vec3& camera, float sizeRatio);

}  // namespace sdl3cpp::services::impl
