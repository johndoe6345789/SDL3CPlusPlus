#pragma once

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_instance.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_kit_gpu.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_state.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_key.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace sdl3cpp::services::impl {

struct Fs2024World;

/// One resident tile, built from FS2024's own data for one level-14
/// quad tile. Its meshes are in the tile's own local space (origin at
/// its north-west corner) and drawn at `offset`, so a long flight never
/// loses float precision; its heightfield's origin is set to `offset`,
/// so collision and ground lookups work in engine space as before.
struct Fs2024LoadedTile {
    Fs2024TerrainState terrain;
    glm::vec3 offset{0.f};  ///< engine position of the tile's local origin
    /// The tile's land classes, one texel per ~24 m (R8, nearest).
    SDL_GPUTexture* classMap = nullptr;
    SDL_GPUSampler* classSampler = nullptr;
    glm::vec4 runway{0.f};      ///< z <= 0 (half-length) means none
    glm::vec4 runwayAxis{0.f};
    /// This tile's buildings, walls and roofs already meshed (same
    /// vertex format terrain uses, so the same pipeline draws both);
    /// indexCount 0 means the tile baked none.
    Fs2024TerrainChunkGpu buildingChunk;
    /// The same buildings' roofs, drawn separately with their own real
    /// roof-tile texture rather than the walls' brick.
    Fs2024TerrainChunkGpu buildingRoofChunk;
    /// The landmarks FS2024 stands in this tile; look up each one's
    /// GPU kit in Fs2024TileStreamState::landmarkKits.
    std::vector<Fs2024LandmarkInstance> landmarks;
};

/// Everything fs2024.tiles.* shares: which tiles are resident, and the
/// streaming radii read once from the package's terrain_root config.
struct Fs2024TileStreamState {
    std::string tilesRoot;
    float tileSize = 1000.f;
    int loadRadiusTiles = 2;
    int evictRadiusTiles = 3;
    int maxLoadsPerCall = 4;

    std::unordered_map<Fs2024TileKey, Fs2024LoadedTile> resident;
    std::vector<Fs2024TileKey> pendingLoad;
    std::vector<Fs2024TileKey> pendingEvict;
    /// Tiles a load attempt found no files for -- outside the baked
    /// area. Left unwanted so resolve does not queue them every call.
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
};

}  // namespace sdl3cpp::services::impl
