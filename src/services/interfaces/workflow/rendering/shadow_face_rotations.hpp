#pragma once

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// The 6 constant face-rotation matrices shared by every shadow-casting
/// box (each face's local +Y becomes that face's outward normal).
struct ShadowFaceRotations {
    glm::mat4 none;
    glm::mat4 down;
    glm::mat4 north;
    glm::mat4 south;
    glm::mat4 east;
    glm::mat4 west;
};

ShadowFaceRotations BuildShadowFaceRotations();

}  // namespace sdl3cpp::services::impl
