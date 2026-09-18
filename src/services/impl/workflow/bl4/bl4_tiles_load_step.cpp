#include "services/interfaces/workflow/bl4/bl4_tiles_load_step.hpp"

#include "services/interfaces/workflow/bl4/bl4_binary_mesh.hpp"
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

#include <algorithm>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

/// Loads (or returns the cached) geometry for one archetype's model
/// path, uploading its GPU buffers and building its collision shape on
/// first use. Failure (bad file, no triangles) is cached too, as an
/// unusable entry, so a broken reference is not retried every load.
/// "models/X.obj" -> "models/X.bmesh", bl4x's binary form of the same mesh.
std::string BinaryMeshPath(const std::string& objPath) {
    const std::size_t dot = objPath.find_last_of('.');
    return (dot == std::string::npos ? objPath : objPath.substr(0, dot)) + ".bmesh";
}

/// The .mtl's map_Kd is relative to the OBJ; the cache key is absolute.
std::string ResolveTexturePath(const std::string& modelPath, const std::string& relative) {
    if (relative.empty()) return {};
    const std::filesystem::path model(modelPath);
    return (model.parent_path() / relative).lexically_normal().generic_string();
}

Bl4SubMesh UploadSubMesh(SDL_GPUDevice* device, Bl4TileStreamState& state,
                         const std::string& fullPath, const Bl4SubMeshData& part,
                         const std::shared_ptr<ILogger>& logger) {
    const BspGeometryBuffers buffers =
        UploadBspGeometryBuffers(device, part.vertices, part.indices);
    Bl4SubMesh sub;
    sub.vertexBuffer = buffers.vertex_buffer;
    sub.indexBuffer = buffers.index_buffer;
    sub.indexCount = static_cast<std::uint32_t>(part.indices.size());
    const std::string texturePath = ResolveTexturePath(fullPath, part.texturePath);
    Bl4Texture* texture = AcquireBl4Texture(state.textureCache, texturePath, device, logger);
    if (texture) {
        sub.texturePath = texturePath;
        sub.texture = texture->texture;
        sub.sampler = texture->sampler;
    }
    return sub;
}

Bl4Geometry* GetOrLoadGeometry(SDL_GPUDevice* device, Bl4TileStreamState& state,
                               const std::string& fullPath,
                               const std::shared_ptr<ILogger>& logger) {
    auto it = state.geometryCache.find(fullPath);
    if (it != state.geometryCache.end()) return &it->second;

    Bl4Geometry geometry;
    geometry.modelPath = fullPath;

    // bl4x bakes a binary form beside each OBJ, already in this engine's
    // vertex layout; the OBJ is the fallback for older bakes.
    Bl4MeshData mesh = LoadBl4BinaryMesh(BinaryMeshPath(fullPath));
    Assimp::Importer importer;
    if (mesh.parts.empty()) {
        constexpr unsigned int kImportFlags =
            aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices;
        const aiScene* scene = importer.ReadFile(fullPath, kImportFlags);
        if (scene && scene->mNumMeshes > 0) mesh = ExtractBl4Mesh(*scene);
    }
    if (!mesh.parts.empty()) {
        for (const Bl4SubMeshData& part : mesh.parts) {
            geometry.subMeshes.push_back(UploadSubMesh(device, state, fullPath, part, logger));
        }
        geometry.boundsCenter = glm::vec3(mesh.boundsCenter[0], mesh.boundsCenter[1],
                                          mesh.boundsCenter[2]);
        geometry.boundsRadius = mesh.boundsRadius;
        geometry.usable = !geometry.subMeshes.empty();
        if (geometry.usable) BuildBl4CollisionShape(mesh, geometry);
    }

    auto [inserted, ok] = state.geometryCache.emplace(fullPath, std::move(geometry));
    (void)ok;
    return &inserted->second;
}

/// The geometry's model-space sphere, placed: its centre through the
/// model matrix, its radius by the largest axis scale.
void SetBl4InstanceBounds(Bl4Instance& instance) {
    const Bl4Geometry& geometry = *instance.geometry;
    if (geometry.boundsRadius < 0.f) return;
    const glm::mat4& model = instance.modelMatrix;
    instance.boundsCenter = glm::vec3(model * glm::vec4(geometry.boundsCenter, 1.f));
    const float scale = std::max({glm::length(glm::vec3(model[0])),
                                  glm::length(glm::vec3(model[1])),
                                  glm::length(glm::vec3(model[2]))});
    instance.boundsRadius = geometry.boundsRadius * scale;
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
        const std::uint64_t identity = Bl4PlacementIdentity(placement);
        if (!state.liveInstances.insert(identity).second) continue;  // a neighbour has it
        const std::string fullPath = state.mapRoot + "/" + placement.modelPath;
        Bl4Geometry* geometry = GetOrLoadGeometry(device, state, fullPath, logger);
        if (!geometry->usable) {
            if (logger) logger->Trace("bl4.tiles.load: unusable model " + fullPath);
            state.liveInstances.erase(identity);
            continue;
        }

        Bl4Instance instance;
        instance.geometry = geometry;
        instance.modelMatrix = InstanceModelMatrix(placement);
        SetBl4InstanceBounds(instance);
        AddBl4InstanceBody(world, placement, *geometry, instance);
        ++geometry->references;
        tile.instances.push_back(instance);
        tile.owned.push_back(identity);
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

    // force loads the whole queue (the warm-up's innermost tiles, so the
    // ground under the spawn exists before the first frame); max_tiles
    // bounds that, since the full map's outer ring costs a minute the
    // player would spend staring at black.
    const bool force = Bl4NumberOr(step, "force", 0.f) != 0.f;
    const auto maxTiles = static_cast<std::size_t>(Bl4NumberOr(step, "max_tiles", 0.f));
    std::size_t budget = force ? state_->pendingLoad.size()
                               : std::min<std::size_t>(state_->maxLoadsPerCall,
                                                       state_->pendingLoad.size());
    if (maxTiles > 0) budget = std::min(budget, maxTiles);

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
