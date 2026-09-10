#include "services/interfaces/workflow/gta5/gta5_collision_shape.hpp"

namespace sdl3cpp::services::impl {

bool BuildGta5CollisionShape(const AssimpMeshData& mesh,
                             Gta5Geometry& geometry) {
    const std::size_t triangles = mesh.indices.size() / 3u;
    if (triangles == 0u || mesh.vertices.empty()) return false;

    geometry.collisionVertices.reserve(mesh.vertices.size() * 3u);
    for (const PosUvVertex& vertex : mesh.vertices) {
        geometry.collisionVertices.push_back(vertex.x);
        geometry.collisionVertices.push_back(vertex.y);
        geometry.collisionVertices.push_back(vertex.z);
    }

    geometry.collisionIndices.reserve(mesh.indices.size());
    for (const std::uint16_t index : mesh.indices) {
        geometry.collisionIndices.push_back(static_cast<int>(index));
    }

    geometry.collisionMesh = new btTriangleIndexVertexArray(
        static_cast<int>(triangles), geometry.collisionIndices.data(),
        static_cast<int>(3 * sizeof(int)),
        static_cast<int>(mesh.vertices.size()),
        geometry.collisionVertices.data(),
        static_cast<int>(3 * sizeof(btScalar)));

    // Static world geometry, so the BVH is worth building once up front.
    geometry.collisionShape =
        new btBvhTriangleMeshShape(geometry.collisionMesh, true);
    return true;
}

void ReleaseGta5CollisionShape(Gta5Geometry& geometry) {
    delete geometry.collisionShape;
    geometry.collisionShape = nullptr;
    delete geometry.collisionMesh;
    geometry.collisionMesh = nullptr;
    geometry.collisionVertices.clear();
    geometry.collisionVertices.shrink_to_fit();
    geometry.collisionIndices.clear();
    geometry.collisionIndices.shrink_to_fit();
}

}  // namespace sdl3cpp::services::impl
