#include "services/interfaces/workflow/gta5/gta5_mesh_append.hpp"

#include <cstdint>

namespace sdl3cpp::services::impl {
namespace {

core::Vertex MakeVertex(const aiMesh& mesh, unsigned int i) {
    core::Vertex vertex{};
    const aiVector3D& position = mesh.mVertices[i];
    vertex.position = {position.x, position.y, position.z};

    if (mesh.HasNormals()) {
        const aiVector3D& normal = mesh.mNormals[i];
        vertex.normal = {normal.x, normal.y, normal.z};
    } else {
        vertex.normal = {0.f, 1.f, 0.f};
    }

    if (mesh.HasTangentsAndBitangents()) {
        const aiVector3D& tangent = mesh.mTangents[i];
        vertex.tangent = {tangent.x, tangent.y, tangent.z};
    } else {
        vertex.tangent = {1.f, 0.f, 0.f};
    }

    if (mesh.HasTextureCoords(0)) {
        const aiVector3D& uv = mesh.mTextureCoords[0][i];
        vertex.texcoord = {uv.x, uv.y};
    } else {
        vertex.texcoord = {0.f, 0.f};
    }

    vertex.color = {1.f, 1.f, 1.f};
    return vertex;
}

}  // namespace

void AppendGta5Mesh(const aiMesh& mesh, Gta5Geometry& geometry) {
    const auto baseVertex =
        static_cast<std::uint16_t>(geometry.vertices.size());

    for (unsigned int i = 0; i < mesh.mNumVertices; ++i) {
        geometry.vertices.push_back(MakeVertex(mesh, i));
    }

    for (unsigned int f = 0; f < mesh.mNumFaces; ++f) {
        const aiFace& face = mesh.mFaces[f];
        // aiProcess_Triangulate guarantees three, but a degenerate face
        // can still arrive with fewer and would index past the end.
        if (face.mNumIndices != 3) continue;
        for (unsigned int i = 0; i < 3; ++i) {
            geometry.indices.push_back(
                static_cast<std::uint16_t>(baseVertex + face.mIndices[i]));
        }
    }
}

std::size_t CountGta5SceneVertices(const aiScene& scene) {
    std::size_t total = 0;
    for (unsigned int m = 0; m < scene.mNumMeshes; ++m) {
        total += scene.mMeshes[m]->mNumVertices;
    }
    return total;
}

}  // namespace sdl3cpp::services::impl
