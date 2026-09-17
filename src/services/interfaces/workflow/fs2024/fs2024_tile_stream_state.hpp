#pragma once

#include "services/interfaces/workflow/fs2024/fs2024_landmark_instance.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_landmark_kit_gpu.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_terrain_state.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_tile_key.hpp"

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace sdl3cpp::services::impl {

/// One resident tile: its ground (heightfield, GPU mesh, collision --
/// Fs2024TerrainState already models exactly one tile's worth of
/// those), plus its own ground texture and, for an airport tile that
/// overlaps it, the runway to draw analytically over that texture.
struct Fs2024LoadedTile {
    Fs2024TerrainState terrain;
    SDL_GPUTexture* groundTexture = nullptr;
    SDL_GPUSampler* groundSampler = nullptr;
    glm::vec4 runway{0.f};      ///< z <= 0 (half-length) means none
    glm::vec4 runwayAxis{0.f};
    /// This tile's buildings, walls and roofs already meshed (same
    /// vertex format terrain uses, so the same pipeline draws both);
    /// indexCount 0 means the tile baked none.
    Fs2024TerrainChunkGpu buildingChunk;
    /// The same buildings' roofs, drawn separately with their own real
    /// roof-tile texture rather than the walls' brick.
    Fs2024TerrainChunkGpu buildingRoofChunk;
    /// This tile's share of `landmarks.json`, if it baked one; look up
    /// each instance's GPU kit in Fs2024TileStreamState::landmarkKits.
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

    /// Every landmark model loaded so far, by its own GXML name --
    /// shared across every tile/instance that references it, loaded
    /// once on first reference and released only at fs2024.tiles.free.
    std::unordered_map<std::string, Fs2024LandmarkKitGpu> landmarkKits;
};

}  // namespace sdl3cpp::services::impl
