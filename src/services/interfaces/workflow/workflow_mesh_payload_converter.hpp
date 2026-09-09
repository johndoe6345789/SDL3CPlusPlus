#pragma once

#include "../interfaces/mesh_types.hpp"
#include "../../core/vertex.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

struct MeshPayloadConversionResult {
    std::vector<core::Vertex> vertices;
    std::vector<uint16_t> indices;
};

MeshPayloadConversionResult ConvertMeshPayload(const MeshPayload& payload);

}  // namespace sdl3cpp::services::impl
