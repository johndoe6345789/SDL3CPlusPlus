#pragma once

#include "core/vertex.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services {

struct SceneObject {
    std::string objectType;
    std::array<float, 16> modelMatrix{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    bool hasCustomModelMatrix = false;
    int computeModelMatrixRef = -1;
    std::vector<core::Vertex> vertices;
    std::vector<uint16_t> indices;
    std::vector<std::string> shaderKeys;
};

}  // namespace sdl3cpp::services
