#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace sdl3cpp::services {

struct MeshPayload {
    std::vector<std::array<float, 3>> positions;
    std::vector<std::array<float, 3>> colors;
    std::vector<uint32_t> indices;
};

}  // namespace sdl3cpp::services
