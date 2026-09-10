#pragma once

#include <assimp/matrix4x4.h>
#include <assimp/mesh.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Vertex format used by map.load's uploaded meshes: float3 pos + float2 uv
/// (20 bytes), matching draw.textured's expected layout.
struct PosUvVertex {
    float x, y, z, u, v;
};

/// One mesh's world-space vertices/indices plus its bounding box, ready to
/// upload to the GPU and/or turn into a static physics body.
struct ExtractedMapMesh {
    std::vector<PosUvVertex> vertices;
    std::vector<uint16_t> indices;
    glm::vec3 bbMin{1e9f, 1e9f, 1e9f};
    glm::vec3 bbMax{-1e9f, -1e9f, -1e9f};
};

/// Applies `transform` and `scale` to every vertex of `mesh`, tracking the
/// resulting bounding box; UVs default to (0,0) when the mesh has none.
ExtractedMapMesh ExtractMapMeshGeometry(const aiMesh* mesh,
                                        const aiMatrix4x4& transform,
                                        float scale);

}  // namespace sdl3cpp::services::impl
