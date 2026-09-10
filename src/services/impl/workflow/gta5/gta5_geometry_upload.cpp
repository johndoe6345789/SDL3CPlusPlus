#include "services/interfaces/workflow/gta5/gta5_geometry_upload.hpp"

#include "services/interfaces/workflow/graphics/graphics_gpu_buffer_upload.hpp"
#include "services/interfaces/workflow/gta5/gta5_collision_shape.hpp"
#include "services/interfaces/workflow/gta5/gta5_mesh_extract.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <cstring>
#include <exception>
#include <vector>

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

    const Gta5MeshData mesh = ExtractGta5Mesh(*scene);
    if (mesh.vertices.empty()) {
        Warn(logger, "'" + placement.modelPath + "' gave no vertices");
        return false;
    }
    if (mesh.vertices.size() >= kGta5MaxVerticesPerMesh) {
        // Truncating would silently corrupt the building, so refuse it.
        Warn(logger, "'" + placement.archetype + "' has " +
                         std::to_string(mesh.vertices.size()) +
                         " vertices, past what a 16-bit index buffer can "
                         "address; skipping rather than truncating");
        return false;
    }

    std::vector<uint8_t> bytes(mesh.vertices.size() *
                               sizeof(BspRenderVertex));
    std::memcpy(bytes.data(), mesh.vertices.data(), bytes.size());
    try {
        // CreateAndUploadGpuBuffers throws on GPU failure. One bad
        // archetype must not take the frame loop down.
        const UploadedGpuBuffers buffers =
            CreateAndUploadGpuBuffers(device, bytes, mesh.indices);
        geometry.vertexBuffer = buffers.vertexBuffer;
        geometry.indexBuffer = buffers.indexBuffer;
    } catch (const std::exception& ex) {
        Warn(logger, "upload failed for '" + placement.archetype + "': " +
                         ex.what());
        return false;
    }

    geometry.indexCount = static_cast<std::uint32_t>(mesh.indices.size());
    // Collide against the drawn triangles: stand on what is visible.
    if (!BuildGta5CollisionShape(mesh, geometry)) {
        Warn(logger, "'" + placement.archetype + "' is not collidable");
    }
    geometry.usable = true;
    return true;
}

}  // namespace sdl3cpp::services::impl
