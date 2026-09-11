#include "services/interfaces/workflow/gta5/gta5_collision_shape.hpp"

namespace sdl3cpp::services::impl {

bool BuildGta5CollisionShape(const Gta5MeshData& mesh,
                             Gta5Geometry& geometry) {
    // Every submesh merged: the player collides with the whole building,
    // not with whichever material happened to come first.
    for (const Gta5SubMeshData& part : mesh.parts) {
        // Cutouts do not collide: a leaf card is a rectangle whose shape
        // is only in its alpha, and as collision a canopy held up a car
        // dropped into the desert. Trunks and branches are solid parts.
        // Nor decals: a tyre track or road marking floats on the road,
        // and at speed the wheels rode over every one.
        if (part.alphaCutoff > 0.f || part.blend) continue;
        const auto base =
            static_cast<int>(geometry.collisionVertices.size() / 3);
        for (const BspRenderVertex& vertex : part.vertices) {
            geometry.collisionVertices.push_back(vertex.x);
            geometry.collisionVertices.push_back(vertex.y);
            geometry.collisionVertices.push_back(vertex.z);
        }
        for (const std::uint16_t index : part.indices) {
            geometry.collisionIndices.push_back(base + static_cast<int>(index));
        }
    }

    const std::size_t triangles = geometry.collisionIndices.size() / 3u;
    if (triangles == 0u || geometry.collisionVertices.empty()) return false;

    geometry.collisionMesh = new btTriangleIndexVertexArray(
        static_cast<int>(triangles), geometry.collisionIndices.data(),
        static_cast<int>(3 * sizeof(int)),
        static_cast<int>(geometry.collisionVertices.size() / 3),
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
