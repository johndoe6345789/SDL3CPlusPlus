#include "services/interfaces/workflow/bl4/bl4_mesh_extract.hpp"

#include <assimp/material.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <limits>

namespace sdl3cpp::services::impl {
namespace {

std::string DiffuseTexturePath(const aiScene& scene, const aiMesh& mesh) {
    if (mesh.mMaterialIndex >= scene.mNumMaterials) return {};
    aiString path;
    const aiMaterial& material = *scene.mMaterials[mesh.mMaterialIndex];
    if (material.GetTexture(aiTextureType_DIFFUSE, 0, &path) != AI_SUCCESS) {
        return {};
    }
    return path.C_Str();
}

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

/// Centre of the parts' AABB, and the radius that covers every vertex
/// from it -- tighter than the AABB's own half-diagonal.
void SetBl4Bounds(Bl4MeshData& mesh) {
    glm::vec3 low(std::numeric_limits<float>::max());
    glm::vec3 high(std::numeric_limits<float>::lowest());
    for (const Bl4SubMeshData& part : mesh.parts) {
        for (const BspRenderVertex& v : part.vertices) {
            low = glm::min(low, glm::vec3(v.x, v.y, v.z));
            high = glm::max(high, glm::vec3(v.x, v.y, v.z));
        }
    }
    if (low.x > high.x) return;
    const glm::vec3 centre = (low + high) * 0.5f;
    float radius = 0.f;
    for (const Bl4SubMeshData& part : mesh.parts) {
        for (const BspRenderVertex& v : part.vertices) {
            radius = std::max(radius, glm::distance(glm::vec3(v.x, v.y, v.z), centre));
        }
    }
    mesh.boundsCenter[0] = centre.x;
    mesh.boundsCenter[1] = centre.y;
    mesh.boundsCenter[2] = centre.z;
    mesh.boundsRadius = radius;
}

Bl4MeshData ExtractBl4Mesh(const aiScene& scene) {
    Bl4MeshData out;
    for (unsigned int m = 0; m < scene.mNumMeshes; ++m) {
        const aiMesh& mesh = *scene.mMeshes[m];
        if (mesh.mNumVertices == 0 || mesh.mNumFaces == 0) continue;
        Bl4SubMeshData part;
        part.texturePath = DiffuseTexturePath(scene, mesh);
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
    SetBl4Bounds(out);
    return out;
}

}  // namespace sdl3cpp::services::impl
