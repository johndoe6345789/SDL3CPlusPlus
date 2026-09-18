#pragma once

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_instance.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_kit_gpu.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_state.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_key.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_lod.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace sdl3cpp::services::impl {

struct Fs2024World;
class Fs2024LoadPool;

/// One resident tile, built from FS2024's own data for one quad tile at
/// any streamed level. Its meshes are in the tile's own local space
/// (origin at its north-west corner) and drawn at `offset`, so a long
/// flight never loses float precision; its heightfield's origin is set
/// to `offset`, so collision and ground lookups work in engine space.
struct Fs2024LoadedTile {
    Fs2024TerrainState terrain;
    glm::vec3 offset{0.f};  ///< engine position of the tile's local origin
    /// The tile's land classes, one texel per ~24 m (R8, nearest).
    SDL_GPUTexture* classMap = nullptr;
    SDL_GPUSampler* classSampler = nullptr;
    /// Its buildings' walls and roofs (apart, for their own textures),
    /// water and roads; indexCount 0 when it has none of one.
    Fs2024TerrainChunkGpu buildingChunk, buildingRoofChunk;
    Fs2024TerrainChunkGpu waterChunk, roadChunk;
    /// The landmarks FS2024 stands in this tile; look up each one's
    /// GPU kit in Fs2024TileStreamState::landmarkKits.
    std::vector<Fs2024LandmarkInstance> landmarks;
};

/// Everything fs2024.tiles.* shares: which tiles are resident, which
/// drawn, which on their way, and how far each detail level reaches.
struct Fs2024TileStreamState {
    float tileSize = 1000.f;  ///< the finest level's width
    Fs2024LodConfig lod;
    float leadSeconds = 1.5f;  ///< stream ahead of the player's travel
    float finishBudgetMs = 4.f;  ///< main-thread upload time per frame

    /// The detail wanted now: the leaves of the latest LOD cut.
    std::unordered_set<Fs2024TileKey> wanted;
    std::unordered_map<Fs2024TileKey, Fs2024LoadedTile> resident;
    /// Resident tiles to draw this frame: never two over one patch.
    std::unordered_set<Fs2024TileKey> drawn;
    std::vector<Fs2024TileKey> pendingLoad;
    std::vector<Fs2024TileKey> pendingEvict;
    /// Queued on, or being built by, the loader threads.
    std::unordered_set<Fs2024TileKey> loading;
    /// Tiles whose build failed. Left unwanted so resolve does not
    /// queue them every call.
    std::unordered_set<Fs2024TileKey> missing;
    bool configured = false;

    /// Every landmark model a resident tile stands, by its own GXML
    /// name: loaded on first use, released once no resident tile uses
    /// it.
    std::unordered_map<std::string, Fs2024LandmarkKitGpu> landmarkKits;

    /// FS2024's own world, once fs2024.world.open has opened it. Every
    /// tile is built from it; nothing streams until it is open. Shared
    /// rather than unique so this header needs only a declaration: the
    /// deleter is fixed where the world is made, and code that never
    /// opens one never has to link it.
    std::shared_ptr<Fs2024World> world;
    /// The loader threads, which read `world`: declared after it, so
    /// they are joined before it goes.
    std::shared_ptr<Fs2024LoadPool> pool;
};

}  // namespace sdl3cpp::services::impl
