#include "services/interfaces/workflow/rendering/model_load_helpers.hpp"

namespace sdl3cpp::services::impl {

AssimpMeshData ExtractAssimpMeshData(const aiScene& scene, float scale) {
    AssimpMeshData out;
    uint16_t baseVertex = 0;

    for (unsigned int m = 0; m < scene.mNumMeshes; ++m) {
        const aiMesh* mesh = scene.mMeshes[m];

        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            PosUvVertex vert;
            vert.x = mesh->mVertices[i].x * scale;
            vert.y = mesh->mVertices[i].y * scale;
            vert.z = mesh->mVertices[i].z * scale;
            if (mesh->mTextureCoords[0]) {
                vert.u = mesh->mTextureCoords[0][i].x;
                vert.v = mesh->mTextureCoords[0][i].y;
            } else {
                vert.u = 0.0f;
                vert.v = 0.0f;
            }
            out.vertices.push_back(vert);
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                out.indices.push_back(
                    static_cast<uint16_t>(baseVertex + face.mIndices[j]));
            }
        }
        baseVertex += static_cast<uint16_t>(mesh->mNumVertices);
    }

    return out;
}

nlohmann::json BuildModelLoadMetadata(uint32_t vertexCount, uint32_t indexCount,
                                      unsigned int meshCount,
                                      const std::string& filePath) {
    return nlohmann::json{
        {"vertex_count", vertexCount},
        {"index_count", indexCount},
        {"stride", 20},
        {"meshes", meshCount},
        {"file", filePath},
    };
}

}  // namespace sdl3cpp::services::impl
