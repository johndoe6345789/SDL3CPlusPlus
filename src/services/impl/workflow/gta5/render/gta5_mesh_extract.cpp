#include "services/interfaces/workflow/gta5/render/gta5_mesh_extract.hpp"

#include "services/interfaces/workflow/gta5/render/gta5_material_surface.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_mesh_vertex.hpp"

#include <assimp/material.h>

#include <filesystem>

namespace sdl3cpp::services::impl {
namespace {

/// glTF puts the map in baseColorTexture, which assimp surfaces as
/// BASE_COLOR and older exporters as DIFFUSE; try both.
std::string TexturePath(const aiScene& scene, const aiMesh& mesh,
                        const std::string& baseDirectory) {
    if (mesh.mMaterialIndex >= scene.mNumMaterials) return "";
    const aiMaterial& material = *scene.mMaterials[mesh.mMaterialIndex];
    aiString found;
    if (material.GetTexture(aiTextureType_BASE_COLOR, 0, &found) !=
            AI_SUCCESS &&
        material.GetTexture(aiTextureType_DIFFUSE, 0, &found) != AI_SUCCESS) {
        return "";
    }
    std::filesystem::path path(found.C_Str());
    if (path.is_absolute() || baseDirectory.empty()) return path.string();
    return (std::filesystem::path(baseDirectory) / path).string();
}

}  // namespace

Gta5MeshData ExtractGta5Mesh(const aiScene& scene,
                             const std::string& baseDirectory) {
    Gta5MeshData out;
    for (unsigned int m = 0; m < scene.mNumMeshes; ++m) {
        const aiMesh& mesh = *scene.mMeshes[m];
        if (mesh.mNumVertices == 0 || mesh.mNumFaces == 0) continue;
        Gta5SubMeshData part;
        part.texturePath = TexturePath(scene, mesh, baseDirectory);
        if (mesh.mMaterialIndex < scene.mNumMaterials) {
            const std::array<float, 4> surface = ReadGta5MaterialSurface(
                *scene.mMaterials[mesh.mMaterialIndex]);
            part.tint = {surface[0], surface[1], surface[2]};
            part.alphaCutoff = surface[3];
        }
        part.vertices.reserve(mesh.mNumVertices);
        for (unsigned int i = 0; i < mesh.mNumVertices; ++i) {
            part.vertices.push_back(MakeGta5Vertex(mesh, i));
        }
        for (unsigned int f = 0; f < mesh.mNumFaces; ++f) {
            // Triangulate guarantees three, but a degenerate face can
            // arrive with fewer and would index past the end.
            const aiFace& face = mesh.mFaces[f];
            if (face.mNumIndices != 3) continue;
            for (unsigned int j = 0; j < 3; ++j) {
                part.indices.push_back(
                    static_cast<std::uint16_t>(face.mIndices[j]));
            }
        }
        if (!part.indices.empty()) out.parts.push_back(std::move(part));
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
