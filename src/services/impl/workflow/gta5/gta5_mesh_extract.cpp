#include "services/interfaces/workflow/gta5/gta5_mesh_extract.hpp"

namespace sdl3cpp::services::impl {
namespace {

BspRenderVertex MakeVertex(const aiMesh& mesh, unsigned int i) {
    BspRenderVertex vertex{};
    const aiVector3D& position = mesh.mVertices[i];
    vertex.x = position.x;
    vertex.y = position.y;
    vertex.z = position.z;

    if (mesh.HasTextureCoords(0)) {
        const aiVector3D& uv = mesh.mTextureCoords[0][i];
        vertex.u = uv.x;
        vertex.v = uv.y;
    }

    // Unused by the gta5 shader, but the vertex format reserves it.
    vertex.lm_u = 0.f;
    vertex.lm_v = 0.f;

    if (mesh.HasNormals()) {
        const aiVector3D& normal = mesh.mNormals[i];
        vertex.nx = normal.x;
        vertex.ny = normal.y;
        vertex.nz = normal.z;
    } else {
        vertex.ny = 1.f;
    }
    return vertex;
}

}  // namespace

Gta5MeshData ExtractGta5Mesh(const aiScene& scene) {
    Gta5MeshData out;
    std::uint16_t baseVertex = 0;

    for (unsigned int m = 0; m < scene.mNumMeshes; ++m) {
        const aiMesh& mesh = *scene.mMeshes[m];
        for (unsigned int i = 0; i < mesh.mNumVertices; ++i) {
            out.vertices.push_back(MakeVertex(mesh, i));
        }

        for (unsigned int f = 0; f < mesh.mNumFaces; ++f) {
            const aiFace& face = mesh.mFaces[f];
            // aiProcess_Triangulate guarantees three, but a degenerate
            // face can still arrive with fewer and would index past the
            // end.
            if (face.mNumIndices != 3) continue;
            for (unsigned int j = 0; j < 3; ++j) {
                out.indices.push_back(
                    static_cast<std::uint16_t>(baseVertex + face.mIndices[j]));
            }
        }
        baseVertex += static_cast<std::uint16_t>(mesh.mNumVertices);
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
