#pragma once

#include <glm/glm.hpp>

#include <array>

namespace sdl3cpp::services::impl {

/// One face of an axis-aligned box: its center-relative offset, outward
/// normal, orientation, and UV tiling in world units.
struct BoxFace {
    glm::vec3 offset;
    glm::vec3 normal;
    glm::mat4 rotation;
    float scaleW, scaleD;
    float uvW, uvH;
};

/// Builds the 6 faces of a `size_x` x `size_y` x `size_z` box centered on
/// the origin, tiling each face's texture at `uvDensity` repeats per unit.
std::array<BoxFace, 6> BuildBoxFaces(float sizeX, float sizeY, float sizeZ,
                                     float uvDensity);

}  // namespace sdl3cpp::services::impl
