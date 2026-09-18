#include "services/interfaces/workflow/fs2024/tiles/fs2024_world_tile_load.hpp"

#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow/fs2024/build/fs2024_tile_building_mesh.hpp"
#include "services/interfaces/workflow/fs2024/build/fs2024_tile_ground.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_clear.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_class_map_upload.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_upload.hpp"
#include "services/interfaces/workflow/rendering/bsp_geometry_upload.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr int kGroundCells = 64;   ///< ~24 m at London, the lcg's own grain
constexpr int kClassTexels = 64;

void UploadChunk(SDL_GPUDevice* device,
                 const std::vector<BspRenderVertex>& vertices,
                 const std::vector<std::uint32_t>& indices,
                 Fs2024TerrainChunkGpu& chunk) {
    if (indices.empty()) return;
    const BspGeometryBuffers buffers =
        UploadBspGeometryBuffers(device, vertices, indices);
    chunk.vertexBuffer = buffers.vertex_buffer;
    chunk.indexBuffer = buffers.index_buffer;
    chunk.indexCount = static_cast<std::uint32_t>(indices.size());
    chunk.min = glm::vec3(1e30f);
    chunk.max = glm::vec3(-1e30f);
    for (const BspRenderVertex& v : vertices) {
        chunk.min = glm::min(chunk.min, glm::vec3(v.x, v.y, v.z));
        chunk.max = glm::max(chunk.max, glm::vec3(v.x, v.y, v.z));
    }
}

}  // namespace

Fs2024LoadedTile LoadFs2024WorldTile(SDL_GPUDevice* device,
                                     btDiscreteDynamicsWorld* physics,
                                     Fs2024World& world,
                                     const Fs2024TileKey& key,
                                     Fs2024LandmarkKits& kits) {
    const float tileSize = world.origin.TileSize();
    int quadX = 0, quadY = 0;
    Fs2024QuadOfKey(world.origin, key, quadX, quadY);

    Fs2024LoadedTile tile;
    tile.offset = glm::vec3(key.x * tileSize, 0.f, key.z * tileSize);
    tile.terrain.field = BuildFs2024TileHeights(*world.dem, quadX, quadY,
                                                tileSize, kGroundCells);
    tile.landmarks = PlaceFs2024TileLandmarks(
        world, quadX, quadY, tile.offset, tile.terrain.field);
    EnsureFs2024LandmarkKits(device, world, tile.landmarks, kits);
    auto plans = PlanFs2024TileBuildings(
        world.buildings->ReadTile(
            sdl3cpp::fs2024::QuadTile{quadX, quadY, kFs2024TileLevel}),
        tileSize);
    DropFs2024BuildingsUnderLandmarks(plans, tile.landmarks, kits);
    const auto mesh = MeshFs2024TileBuildings(plans, tile.terrain.field);
    UploadChunk(device, mesh.wallVertices, mesh.wallIndices,
                tile.buildingChunk);
    UploadChunk(device, mesh.roofVertices, mesh.roofIndices,
                tile.buildingRoofChunk);

    // Meshes stay tile-local; collision works in engine space.
    UploadFs2024TerrainChunks(device, tile.terrain, kGroundCells);
    tile.terrain.field.origin = glm::vec2(tile.offset.x, tile.offset.z);
    tile.terrain.collision = AddFs2024TerrainCollision(physics,
                                                       tile.terrain.field);
    tile.terrain.loaded = true;

    const auto map = UploadFs2024ClassMap(
        device,
        BuildFs2024TileClasses(*world.classes, quadX, quadY, kClassTexels),
        kClassTexels);
    tile.classMap = map.texture;
    tile.classSampler = map.sampler;
    return tile;
}

}  // namespace sdl3cpp::services::impl
