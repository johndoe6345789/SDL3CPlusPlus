#include "services/interfaces/workflow/gta5/gta5_geometry_upload.hpp"

#include "services/interfaces/workflow/gta5/gta5_collision_shape.hpp"
#include "services/interfaces/workflow/gta5/gta5_mesh_extract.hpp"
#include "services/interfaces/workflow/gta5/gta5_submesh_upload.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <exception>
#include <filesystem>

namespace sdl3cpp::services::impl {
namespace {

constexpr unsigned int kImportFlags =
    aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs |
    aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices;

void Warn(const std::shared_ptr<ILogger>& logger, const std::string& what) {
    if (logger) logger->Warn("gta5.tiles.load: " + what);
}

}  // namespace

bool BuildGta5Geometry(const Gta5Placement& placement, SDL_GPUDevice* device,
                       Gta5GeometryArena& arena, Gta5TextureCache& textures,
                       Gta5Geometry& geometry,
                       const std::shared_ptr<ILogger>& logger) {
    Assimp::Importer importer;
    const aiScene* scene =
        importer.ReadFile(placement.modelPath, kImportFlags);
    if (!scene || !scene->mRootNode ||
        (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0) {
        Warn(logger, "cannot read '" + placement.modelPath + "': " +
                         importer.GetErrorString());
        return false;
    }

    const std::string base =
        std::filesystem::path(placement.modelPath).parent_path().string();
    const Gta5MeshData mesh = ExtractGta5Mesh(*scene, base);
    if (mesh.parts.empty()) {
        Warn(logger, "'" + placement.modelPath + "' gave no geometry");
        return false;
    }

    for (const Gta5SubMeshData& part : mesh.parts) {
        Gta5SubMesh sub;
        try {
            // One oversized or unuploadable material must not cost the
            // rest of the building.
            if (UploadGta5SubMesh(part, device, arena, textures, sub,
                                  logger)) {
                geometry.subMeshes.push_back(sub);
            }
        } catch (const std::exception& ex) {
            Warn(logger, "upload failed for '" + placement.archetype +
                             "': " + ex.what());
        }
    }
    if (geometry.subMeshes.empty()) return false;

    // Collide against the drawn triangles: stand on what is visible.
    if (!BuildGta5CollisionShape(mesh, geometry)) {
        Warn(logger, "'" + placement.archetype + "' is not collidable");
    }
    geometry.usable = true;
    return true;
}

}  // namespace sdl3cpp::services::impl
