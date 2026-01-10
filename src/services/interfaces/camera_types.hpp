#pragma once

#include <array>

namespace sdl3cpp::services {

struct CameraPose {
    std::array<float, 3> position{0.0f, 0.0f, 5.0f};
    std::array<float, 3> lookAt{0.0f, 0.0f, 0.0f};
    std::array<float, 3> up{0.0f, 1.0f, 0.0f};
    float fovDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
};

}  // namespace sdl3cpp::services
