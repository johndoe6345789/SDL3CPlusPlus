#pragma once

#include "../../core/vertex.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services {

struct SceneObject {
    std::vector<core::Vertex> vertices;
    std::vector<uint16_t> indices;
    int computeModelMatrixRef = -1;
    std::array<float, 16> modelMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    bool hasCustomModelMatrix = false;
    std::vector<std::string> shaderKeys;
    std::string objectType;  // Semantic object type (e.g., "lantern", "physics_cube", "floor")
};

}  // namespace sdl3cpp::services
