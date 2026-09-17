#include "services/interfaces/workflow/bl4/bl4_collision_shape.hpp"

namespace sdl3cpp::services::impl {

bool BuildBl4CollisionShape(const Bl4MeshData& mesh, Bl4Geometry& geometry) {
    // Every submesh merged: the player collides with the whole mesh, not
    // with whichever material section happened to come first.
    for (const Bl4SubMeshData& part : mesh.parts) {
        const auto base = static_cast<int>(geometry.collisionVertices.size() / 3);
        for (const BspRenderVertex& vertex : part.vertices) {
            geometry.collisionVertices.push_back(vertex.x);
            geometry.collisionVertices.push_back(vertex.y);
            geometry.collisionVertices.push_back(vertex.z);
        }
        for (const std::uint32_t index : part.indices) {
            geometry.collisionIndices.push_back(base + static_cast<int>(index));
        }
    }

    const std::size_t triangles = geometry.collisionIndices.size() / 3u;
    if (triangles == 0u || geometry.collisionVertices.empty()) return false;

    geometry.collisionMesh = new btTriangleIndexVertexArray(
        static_cast<int>(triangles), geometry.collisionIndices.data(),
        static_cast<int>(3 * sizeof(int)),
        static_cast<int>(geometry.collisionVertices.size() / 3),
        geometry.collisionVertices.data(), static_cast<int>(3 * sizeof(btScalar)));

    // Static world geometry, so the BVH is worth building once up front.
    geometry.collisionShape = new btBvhTriangleMeshShape(geometry.collisionMesh, true);
    return true;
}

void ReleaseBl4CollisionShape(Bl4Geometry& geometry) {
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
