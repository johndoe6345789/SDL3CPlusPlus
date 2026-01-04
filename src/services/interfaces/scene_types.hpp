#pragma once

#include "../../core/vertex.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services {

struct SceneObject {
    std::vector<core::Vertex> vertices;
    std::vector<uint16_t> indices;
    int computeModelMatrixRef = -1;
    std::string shaderKey = "default";
};

}  // namespace sdl3cpp::services
