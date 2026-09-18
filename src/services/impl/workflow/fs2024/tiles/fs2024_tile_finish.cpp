#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_finish.hpp"

#include "services/interfaces/workflow/fs2024/terrain/fs2024_class_map_upload.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow/rendering/bsp_geometry_upload.hpp"

namespace sdl3cpp::services::impl {
namespace {

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

Fs2024LoadedTile FinishFs2024Tile(SDL_GPUDevice* device,
                                  btDiscreteDynamicsWorld* physics,
                                  Fs2024World& world,
                                  Fs2024PreparedTile& prepared,
                                  Fs2024LandmarkKits& kits) {
    Fs2024LoadedTile tile;
    tile.offset = Fs2024TileCorner(prepared.key, world.origin.TileSize());
    Fs2024TerrainChunkGpu ground;
    UploadChunk(device, prepared.ground.vertices, prepared.ground.indices,
                ground);
    if (ground.indexCount) tile.terrain.chunks.push_back(ground);
    UploadChunk(device, prepared.buildings.wallVertices,
                prepared.buildings.wallIndices, tile.buildingChunk);
    UploadChunk(device, prepared.buildings.roofVertices,
                prepared.buildings.roofIndices, tile.buildingRoofChunk);

    // Meshes stay tile-local; collision works in engine space.
    tile.terrain.field = std::move(prepared.field);
    tile.terrain.field.origin = glm::vec2(tile.offset.x, tile.offset.z);
    tile.terrain.collision =
        AddFs2024TerrainCollision(physics, tile.terrain.field);
    tile.terrain.loaded = true;

    const auto map =
        UploadFs2024ClassMap(device, prepared.classes, prepared.classSize);
    tile.classMap = map.texture;
    tile.classSampler = map.sampler;

    AdoptFs2024TileKits(device, world, prepared, kits);
    tile.landmarks = std::move(prepared.landmarks);
    return tile;
}

}  // namespace sdl3cpp::services::impl
