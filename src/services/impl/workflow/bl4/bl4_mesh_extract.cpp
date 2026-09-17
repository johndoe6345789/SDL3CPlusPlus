#include "services/interfaces/workflow/bl4/bl4_mesh_extract.hpp"

namespace sdl3cpp::services::impl {
namespace {

BspRenderVertex MakeBl4Vertex(const aiMesh& mesh, unsigned int index) {
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

}  // namespace

Bl4MeshData ExtractBl4Mesh(const aiScene& scene) {
    Bl4MeshData out;
    for (unsigned int m = 0; m < scene.mNumMeshes; ++m) {
        const aiMesh& mesh = *scene.mMeshes[m];
        if (mesh.mNumVertices == 0 || mesh.mNumFaces == 0) continue;
        Bl4SubMeshData part;
        part.vertices.reserve(mesh.mNumVertices);
        for (unsigned int i = 0; i < mesh.mNumVertices; ++i) {
            part.vertices.push_back(MakeBl4Vertex(mesh, i));
        }
        for (unsigned int f = 0; f < mesh.mNumFaces; ++f) {
            // Triangulate guarantees three, but a degenerate face can
            // arrive with fewer and would index past the end.
            const aiFace& face = mesh.mFaces[f];
            if (face.mNumIndices != 3) continue;
            for (unsigned int j = 0; j < 3; ++j) part.indices.push_back(face.mIndices[j]);
        }
        if (!part.indices.empty()) out.parts.push_back(std::move(part));
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
