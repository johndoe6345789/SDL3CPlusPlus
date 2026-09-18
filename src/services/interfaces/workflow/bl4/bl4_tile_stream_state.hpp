#pragma once

#include "services/interfaces/workflow/bl4/bl4_geometry.hpp"
#include "services/interfaces/workflow/bl4/bl4_instance_batch.hpp"
#include "services/interfaces/workflow/bl4/bl4_texture_cache.hpp"
#include "services/interfaces/workflow/bl4/bl4_tile_key.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace sdl3cpp::services::impl {

/// One resident tile: every instance bl4x placed within it.
struct Bl4LoadedTile {
    std::vector<Bl4Instance> instances;
    /// Identities of the instances this tile owns. bl4x lists a
    /// placement in every tile its mesh reaches -- a 250 m landscape
    /// mesh spans sixteen 64 m tiles -- so the first tile to load one
    /// owns it and the neighbours skip it.
    std::vector<std::uint64_t> owned;
};

/// Everything bl4.tiles.*/bl4.models.draw share: which tiles are
/// resident, the streaming radii read once from the package's
/// map_root config, and the per-archetype geometry cache shared across
/// every tile (mirrors packages/fs2024's Fs2024TileStreamState /
/// packages/gta5's geometry cache).
struct Bl4TileStreamState {
    std::string mapRoot;
    float tileSize = 64.f;
    int loadRadiusTiles = 2;
    int evictRadiusTiles = 3;
    int maxLoadsPerCall = 4;

    std::unordered_map<Bl4TileKey, Bl4LoadedTile> resident;
    /// Every instance identity currently resident, from whichever tile
    /// loaded it first.
    std::unordered_set<std::uint64_t> liveInstances;
    std::vector<Bl4TileKey> pendingLoad;
    std::vector<Bl4TileKey> pendingEvict;
    /// Tiles a load attempt found no placements.json for -- outside the
    /// baked region. Left unwanted so resolve does not queue them every
    /// call.
    std::unordered_set<Bl4TileKey> missing;
    /// Keyed by absolute model path; ref-counted across every tile/
    /// instance that shares an archetype (most BL4 props and foliage
    /// are reused across many placements).
    std::unordered_map<std::string, Bl4Geometry> geometryCache;
    /// Keyed by absolute image path; ref-counted per submesh.
    Bl4TextureCache textureCache;
    /// This frame's visible instances, rebuilt by bl4.models.draw.
    Bl4InstanceBatch batch;
    bool configured = false;
};

}  // namespace sdl3cpp::services::impl
