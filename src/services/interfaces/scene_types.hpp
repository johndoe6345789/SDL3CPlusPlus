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
    std::vector<std::string> shaderKeys;
};

}  // namespace sdl3cpp::services
