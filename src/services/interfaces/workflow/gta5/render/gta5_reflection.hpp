#pragma once

#include "services/interfaces/workflow/gta5/world/gta5_water.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

/// The height of the water the camera is most likely looking at: of the
/// water quads at or below the eye, the nearest across the ground -- the
/// sea at 0 m, or a lake up in the hills. The mirror goes there.
float ChooseGta5MirrorHeight(const std::vector<Gta5WaterQuad>& water,
                             const glm::vec3& eye);

/// A reflection in the level plane y = height.
glm::mat4 Gta5MirrorAt(float height);

/// `proj` with its near plane moved onto `plane` (view space; the side
/// kept is where the plane is positive). Drawn so, what lies below the
/// water is clipped from the mirrored view, and a lake's bed does not
/// rise into its own reflection. Lengyel's oblique near plane, 0..1 depth.
glm::mat4 Gta5ObliqueProjection(const glm::mat4& proj,
                                const glm::vec4& plane);

}  // namespace sdl3cpp::services::impl
