#include "services/interfaces/workflow/gta5/gta5_mesh_import.hpp"

#include "services/interfaces/workflow/gta5/gta5_mesh_append.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace sdl3cpp::services::impl {
namespace {

constexpr unsigned int kImportFlags =
    aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs |
    aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices;

}  // namespace

bool ImportGta5Mesh(const std::string& path, const std::string& archetype,
                    Gta5Geometry& outGeometry,
                    const std::shared_ptr<ILogger>& logger) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, kImportFlags);
    if (!scene || !scene->mRootNode ||
        (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0) {
        if (logger) {
            logger->Warn("gta5.tiles.load: failed to load '" + path +
                         "': " + importer.GetErrorString());
        }
        return false;
    }

    const std::size_t vertices = CountGta5SceneVertices(*scene);
    if (vertices == 0) {
        if (logger) {
            logger->Warn("gta5.tiles.load: '" + path + "' has no vertices");
        }
        return false;
    }

    if (vertices >= kGta5MaxVerticesPerObject) {
        // Truncating would silently corrupt the building, so refuse it and
        // say why. Splitting across several SceneObjects is the real fix
        // and needs the renderer to grow a submesh concept.
        if (logger) {
            logger->Warn("gta5.tiles.load: archetype '" + archetype +
                         "' has " + std::to_string(vertices) +
                         " vertices, over the 65536 a SceneObject can "
                         "index; skipping it rather than truncating");
        }
        return false;
    }

    outGeometry.vertices.reserve(vertices);
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        AppendGta5Mesh(*scene->mMeshes[m], outGeometry);
    }
    outGeometry.usable = true;
    return true;
}

}  // namespace sdl3cpp::services::impl
