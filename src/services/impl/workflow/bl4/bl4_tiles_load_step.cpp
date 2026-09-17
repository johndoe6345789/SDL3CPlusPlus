#include "services/interfaces/workflow/bl4/bl4_tiles_load_step.hpp"

#include "services/interfaces/workflow/bl4/bl4_collision_body.hpp"
#include "services/interfaces/workflow/bl4/bl4_collision_shape.hpp"
#include "services/interfaces/workflow/bl4/bl4_mesh_extract.hpp"
#include "services/interfaces/workflow/bl4/bl4_placement_parse.hpp"
#include "services/interfaces/workflow/bl4/bl4_step_params.hpp"
#include "services/interfaces/workflow/rendering/bsp_geometry_upload.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

/// Loads (or returns the cached) geometry for one archetype's model
/// path, uploading its GPU buffers and building its collision shape on
/// first use. Failure (bad file, no triangles) is cached too, as an
/// unusable entry, so a broken reference is not retried every load.
Bl4Geometry* GetOrLoadGeometry(SDL_GPUDevice* device, Bl4TileStreamState& state,
                               const std::string& fullPath) {
    auto it = state.geometryCache.find(fullPath);
    if (it != state.geometryCache.end()) return &it->second;

    Bl4Geometry geometry;
    geometry.modelPath = fullPath;

    Assimp::Importer importer;
    constexpr unsigned int kImportFlags =
        aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices;
    const aiScene* scene = importer.ReadFile(fullPath, kImportFlags);
    if (scene && scene->mNumMeshes > 0) {
        const Bl4MeshData mesh = ExtractBl4Mesh(*scene);
        for (const Bl4SubMeshData& part : mesh.parts) {
            const BspGeometryBuffers buffers =
                UploadBspGeometryBuffers(device, part.vertices, part.indices);
            geometry.subMeshes.push_back(
                {buffers.vertex_buffer, buffers.index_buffer,
                 static_cast<std::uint32_t>(part.indices.size())});
        }
        geometry.usable = !geometry.subMeshes.empty();
        if (geometry.usable) BuildBl4CollisionShape(mesh, geometry);
    }

    auto [inserted, ok] = state.geometryCache.emplace(fullPath, std::move(geometry));
    (void)ok;
    return &inserted->second;
}

glm::mat4 InstanceModelMatrix(const Bl4Placement& placement) {
    glm::mat4 t = glm::translate(glm::mat4(1.f), placement.position);
    t *= glm::mat4_cast(placement.rotation);
    t = glm::scale(t, placement.scale);
    return t;
}

/// Everything one tile needs. Throws when placements.json is missing or
/// unparsable; a placement whose own model failed to load is skipped
/// (logged once per tile) rather than failing the whole tile.
Bl4LoadedTile LoadOneTile(SDL_GPUDevice* device, btDiscreteDynamicsWorld* world,
                         Bl4TileStreamState& state, const std::string& dir,
                         const std::shared_ptr<ILogger>& logger) {
    std::ifstream in(dir + "/placements.json");
    if (!in) throw std::runtime_error("no placements.json in " + dir);
    const nlohmann::json doc = nlohmann::json::parse(in);

    Bl4LoadedTile tile;
    for (const Bl4Placement& placement : ParseBl4TilePlacements(doc)) {
        if (placement.modelPath.empty()) continue;
        const std::string fullPath = state.mapRoot + "/" + placement.modelPath;
        Bl4Geometry* geometry = GetOrLoadGeometry(device, state, fullPath);
        if (!geometry->usable) {
            if (logger) logger->Trace("bl4.tiles.load: unusable model " + fullPath);
            continue;
        }

        Bl4Instance instance;
        instance.geometry = geometry;
        instance.modelMatrix = InstanceModelMatrix(placement);
        AddBl4InstanceBody(world, placement, *geometry, instance);
        ++geometry->references;
        tile.instances.push_back(instance);
    }
    return tile;
}

}  // namespace

WorkflowBl4TilesLoadStep::WorkflowBl4TilesLoadStep(std::shared_ptr<ILogger> logger,
                                                  std::shared_ptr<Bl4TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowBl4TilesLoadStep::GetPluginId() const { return "bl4.tiles.load"; }

void WorkflowBl4TilesLoadStep::Execute(const WorkflowStepDefinition& step,
                                       WorkflowContext& context) {
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* world = context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!device || state_->pendingLoad.empty()) return;

    const bool force = Bl4NumberOr(step, "force", 0.f) != 0.f;
    const std::size_t budget =
        force ? state_->pendingLoad.size()
              : std::min<std::size_t>(state_->maxLoadsPerCall, state_->pendingLoad.size());

    for (std::size_t i = 0; i < budget; ++i) {
        const Bl4TileKey key = state_->pendingLoad[i];
        const std::string dir = Bl4TileDirectory(state_->mapRoot, key);
        try {
            state_->resident.emplace(key, LoadOneTile(device, world, *state_, dir, logger_));
        } catch (const std::exception& error) {
            state_->missing.insert(key);
            if (logger_) {
                logger_->Trace("bl4.tiles.load: no tile at (" + std::to_string(key.x) + ", " +
                               std::to_string(key.z) + "): " + error.what());
            }
        }
    }
    state_->pendingLoad.erase(state_->pendingLoad.begin(),
                              state_->pendingLoad.begin() + static_cast<long>(budget));
}

}  // namespace sdl3cpp::services::impl
