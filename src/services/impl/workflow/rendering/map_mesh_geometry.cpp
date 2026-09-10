#include "services/interfaces/workflow/rendering/map_mesh_geometry.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

ExtractedMapMesh ExtractMapMeshGeometry(const aiMesh* mesh,
                                        const aiMatrix4x4& transform,
                                        float scale) {
    ExtractedMapMesh result;
    result.vertices.reserve(mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        aiVector3D pos = transform * mesh->mVertices[i];
        pos *= scale;

        PosUvVertex vert;
        vert.x = pos.x;
        vert.y = pos.y;
        vert.z = pos.z;
        if (mesh->mTextureCoords[0]) {
            vert.u = mesh->mTextureCoords[0][i].x;
            vert.v = mesh->mTextureCoords[0][i].y;
        } else {
            vert.u = 0.0f;
            vert.v = 0.0f;
        }
        result.vertices.push_back(vert);

        result.bbMin.x = std::min(result.bbMin.x, pos.x);
        result.bbMin.y = std::min(result.bbMin.y, pos.y);
        result.bbMin.z = std::min(result.bbMin.z, pos.z);
        result.bbMax.x = std::max(result.bbMax.x, pos.x);
        result.bbMax.y = std::max(result.bbMax.y, pos.y);
        result.bbMax.z = std::max(result.bbMax.z, pos.z);
    }

    for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
        const aiFace& face = mesh->mFaces[f];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            result.indices.push_back(static_cast<uint16_t>(face.mIndices[j]));
        }
    }
    return result;
}

}  // namespace sdl3cpp::services::impl
