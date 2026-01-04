#ifndef SDL3CPP_SCRIPT_MESH_PAYLOAD_HPP
#define SDL3CPP_SCRIPT_MESH_PAYLOAD_HPP

#include <array>
#include <cstdint>
#include <vector>

namespace sdl3cpp::script {

struct MeshPayload {
    std::vector<std::array<float, 3>> positions;
    std::vector<std::array<float, 3>> colors;
    std::vector<uint32_t> indices;
};

}  // namespace sdl3cpp::script

#endif  // SDL3CPP_SCRIPT_MESH_PAYLOAD_HPP
