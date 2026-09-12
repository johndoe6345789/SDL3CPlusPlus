#include "services/interfaces/workflow/gta5/render/gta5_mesh_vertex.hpp"

namespace sdl3cpp::services::impl {

BspRenderVertex MakeGta5Vertex(const aiMesh& mesh, unsigned int index) {
    BspRenderVertex vertex{};
    const aiVector3D& position = mesh.mVertices[index];
    vertex.x = position.x;
    vertex.y = position.y;
    vertex.z = position.z;

    if (mesh.HasTextureCoords(0)) {
        const aiVector3D& uv = mesh.mTextureCoords[0][index];
        vertex.u = uv.x;
        vertex.v = uv.y;
    }

    if (mesh.HasNormals()) {
        const aiVector3D& normal = mesh.mNormals[index];
        vertex.nx = normal.x;
        vertex.ny = normal.y;
        vertex.nz = normal.z;
    } else {
        // Anything is better than a zero normal, which shades black.
        vertex.ny = 1.f;
    }
    return vertex;
}

}  // namespace sdl3cpp::services::impl
