#pragma once

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// World-space position and (normalized) direction for a spotlight.
struct SpotlightPose {
    glm::vec3 position;
    glm::vec3 direction;
};

}  // namespace sdl3cpp::services::impl
