#include "workflow_mesh_payload_converter.hpp"

#include <limits>
#include <stdexcept>

namespace sdl3cpp::services::impl {

MeshPayloadConversionResult ConvertMeshPayload(const MeshPayload& payload) {
    MeshPayloadConversionResult result;
    const size_t vertexCount = payload.positions.size();
    result.vertices.reserve(vertexCount);

    for (size_t i = 0; i < vertexCount; ++i) {
        core::Vertex vertex{};
        vertex.position = payload.positions[i];
        if (i < payload.normals.size()) {
            vertex.normal = payload.normals[i];
        }
        if (i < payload.tangents.size()) {
            vertex.tangent = payload.tangents[i];
        }
        if (i < payload.texcoords.size()) {
            vertex.texcoord = payload.texcoords[i];
        }
        if (i < payload.colors.size()) {
            vertex.color = payload.colors[i];
        } else {
            vertex.color = {1.0f, 1.0f, 1.0f};
        }
        result.vertices.push_back(vertex);
    }

    constexpr uint32_t kMaxIndex = std::numeric_limits<uint16_t>::max();
    result.indices.reserve(payload.indices.size());
    for (uint32_t index : payload.indices) {
        if (index > kMaxIndex) {
            throw std::runtime_error("Mesh indices exceed uint16_t range");
        }
        result.indices.push_back(static_cast<uint16_t>(index));
    }

    return result;
}

}  // namespace sdl3cpp::services::impl
