#pragma once

#include "services/interfaces/workflow/gta5/stream/gta5_geometry.hpp"

#include <glm/glm.hpp>

#include <array>

namespace sdl3cpp::services::impl {

/// The six planes of the view volume, normals pointing inwards.
struct Gta5Frustum {
    std::array<glm::vec4, 6> planes{};
};

Gta5Frustum MakeGta5Frustum(const glm::mat4& viewProj);

/// Whether an instance's bounding sphere reaches into the view, and is
/// big enough there to matter: `sizeRatio` culls one whose radius is
/// below that fraction of its distance -- a bin at 300 m is under a
/// pixel. An instance whose geometry has no bounds is always drawn. Its
/// LOD range, scaled by `lodScale`, must hold the camera too.
bool Gta5InstanceVisible(const Gta5Frustum& frustum,
                         const Gta5Instance& instance,
                         const glm::vec3& camera, float sizeRatio,
                         float lodScale = 1.f);

}  // namespace sdl3cpp::services::impl
