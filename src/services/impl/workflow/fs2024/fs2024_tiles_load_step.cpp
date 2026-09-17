#include "services/interfaces/workflow/fs2024/fs2024_tiles_load_step.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_building_mesh.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_landmark_load.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_terrain_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_gpu_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_image_io.hpp"
#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"
#include "services/interfaces/workflow/rendering/bsp_geometry_upload.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

/// The optional "runway" object in a tile's roads.json, or an inactive
/// (z <= 0) runway when the tile has none or the file has no such key.
void ReadTileRunway(const std::string& dir, Fs2024LoadedTile& tile) {
    std::ifstream in(dir + "/roads.json");
    const auto doc = nlohmann::json::parse(in, nullptr, false);
    if (!doc.is_object() || !doc.contains("runway")) return;
    const auto& r = doc["runway"];
    const float length = r.value("length", 0.f);
    const float heading = r.value("heading", 0.f) * 3.14159265f / 180.f;
    tile.runway = glm::vec4(r.value("x", 0.f), r.value("z", 0.f),
                            0.5f * length, 0.5f * r.value("width", 45.f));
    tile.runwayAxis = glm::vec4(std::sin(heading), -std::cos(heading), 0.f,
                                0.f);
}

void LoadTileGroundTexture(SDL_GPUDevice* device, const std::string& dir,
                          Fs2024LoadedTile& tile) {
    LoadedTextureImage image = LoadTextureImagePixels(dir + "/ground.png");
    const UploadedTexture uploaded = UploadTextureImage(device, image);
    tile.groundTexture = uploaded.texture;
    tile.groundSampler =
        CreateTextureLoadSampler(device, uploaded.texture, uploaded.numLevels);
}

/// Reads a tile's buildings.fsb (footprint + height records, in
/// engine-space metres) and meshes them all into one vertex/index
/// list -- a tile's buildings share one draw call, the way its ground
/// blocks each get their own but its whole building set does not need
/// to, since nothing needs to cull within a single small tile.
void LoadTileBuildings(SDL_GPUDevice* device, const std::string& dir,
                     Fs2024LoadedTile& tile) {
    std::ifstream in(dir + "/buildings.fsb", std::ios::binary);
    char magic[4];
    in.read(magic, 4);
    if (!in || std::memcmp(magic, "FSB1", 4) != 0) return;

    std::uint32_t count = 0;
    in.read(reinterpret_cast<char*>(&count), 4);
    std::vector<BspRenderVertex> wallVertices, roofVertices;
    std::vector<std::uint32_t> wallIndices, roofIndices;
    glm::vec3 min(1e30f), max(-1e30f);

    for (std::uint32_t i = 0; i < count && in; ++i) {
        std::uint32_t points = 0;
        float height = 0.f;
        in.read(reinterpret_cast<char*>(&points), 4);
        in.read(reinterpret_cast<char*>(&height), 4);
        std::vector<Point2> footprint(points);
        for (Point2& p : footprint) {
            in.read(reinterpret_cast<char*>(&p.x), 4);
            in.read(reinterpret_cast<char*>(&p.y), 4);
            min.x = std::min(min.x, p.x); max.x = std::max(max.x, p.x);
            min.z = std::min(min.z, p.y); max.z = std::max(max.z, p.y);
        }
        max.y = std::max(max.y, height);
        AppendBuildingMesh(footprint, height, wallVertices, wallIndices,
                          roofVertices, roofIndices);
    }
    if (wallIndices.empty() && roofIndices.empty()) return;

    min.y = 0.f;
    if (!wallIndices.empty()) {
        const BspGeometryBuffers buffers =
            UploadBspGeometryBuffers(device, wallVertices, wallIndices);
        tile.buildingChunk.vertexBuffer = buffers.vertex_buffer;
        tile.buildingChunk.indexBuffer = buffers.index_buffer;
        tile.buildingChunk.indexCount =
            static_cast<std::uint32_t>(wallIndices.size());
        tile.buildingChunk.min = min;
        tile.buildingChunk.max = max;
    }
    if (!roofIndices.empty()) {
        const BspGeometryBuffers buffers =
            UploadBspGeometryBuffers(device, roofVertices, roofIndices);
        tile.buildingRoofChunk.vertexBuffer = buffers.vertex_buffer;
        tile.buildingRoofChunk.indexBuffer = buffers.index_buffer;
        tile.buildingRoofChunk.indexCount =
            static_cast<std::uint32_t>(roofIndices.size());
        tile.buildingRoofChunk.min = min;
        tile.buildingRoofChunk.max = max;
    }
}

/// Loads a tile's own landmark instances and, for any model not
/// already in the shared cache, its GPU kit -- shared, global data
/// loaded once regardless of how many tiles/instances reference it.
void LoadTileLandmarks(SDL_GPUDevice* device, const std::string& dir,
                      Fs2024TileStreamState& state,
                      Fs2024LoadedTile& tile) {
    tile.landmarks = ReadTileLandmarks(dir);
    for (const Fs2024LandmarkInstance& instance : tile.landmarks) {
        if (state.landmarkKits.contains(instance.model)) continue;
        state.landmarkKits[instance.model] = LoadFs2024LandmarkKitGpu(
            device, state.tilesRoot, instance.model, instance.x, instance.z,
            instance.headingDegrees);
    }
}

/// Everything one tile needs. Throws on a missing/unreadable
/// terrain.fst; a missing ground.jpg or roads.json is tolerated (a
/// plain-coloured tile beats no ground at all).
Fs2024LoadedTile LoadOneTile(SDL_GPUDevice* device,
                            btDiscreteDynamicsWorld* world,
                            const std::string& dir,
                            Fs2024TileStreamState& state) {
    Fs2024LoadedTile tile;
    tile.terrain.field = ReadFs2024Heightfield(dir + "/terrain.fst");
    UploadFs2024TerrainChunks(device, tile.terrain, 64);
    tile.terrain.collision =
        AddFs2024TerrainCollision(world, tile.terrain.field);
    tile.terrain.loaded = true;
    ReadTileRunway(dir, tile);
    try {
        LoadTileGroundTexture(device, dir, tile);
    } catch (const std::exception&) {
        // No texture: the tile still stands and collides, just bare.
    }
    try {
        LoadTileBuildings(device, dir, tile);
    } catch (const std::exception&) {
        // No buildings: fine, most tiles worldwide will have none baked.
    }
    try {
        LoadTileLandmarks(device, dir, state, tile);
    } catch (const std::exception&) {
        // No landmarks: the common case, everywhere but a few spots.
    }
    return tile;
}

}  // namespace

WorkflowFs2024TilesLoadStep::WorkflowFs2024TilesLoadStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024TilesLoadStep::GetPluginId() const {
    return "fs2024.tiles.load";
}

void WorkflowFs2024TilesLoadStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!device || state_->pendingLoad.empty()) return;

    const bool force = Fs2024NumberOr(step, "force", 0.f) != 0.f;
    const std::size_t budget =
        force ? state_->pendingLoad.size()
              : std::min<std::size_t>(state_->maxLoadsPerCall,
                                      state_->pendingLoad.size());

    for (std::size_t i = 0; i < budget; ++i) {
        const Fs2024TileKey key = state_->pendingLoad[i];
        const std::string dir =
            Fs2024TileDirectory(state_->tilesRoot, key);
        try {
            state_->resident.emplace(
                key, LoadOneTile(device, world, dir, *state_));
        } catch (const std::exception& error) {
            state_->missing.insert(key);
            if (logger_) {
                logger_->Trace("fs2024.tiles.load: no tile at (" +
                               std::to_string(key.x) + ", " +
                               std::to_string(key.z) + "): " +
                               error.what());
            }
        }
    }
    state_->pendingLoad.erase(state_->pendingLoad.begin(),
                              state_->pendingLoad.begin() +
                                  static_cast<long>(budget));
}

}  // namespace sdl3cpp::services::impl
