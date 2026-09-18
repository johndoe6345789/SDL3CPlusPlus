#pragma once

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_clear.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_prepared_tile.hpp"

namespace sdl3cpp::services::impl {

struct Fs2024World;

/// Builds tile `key` from FS2024's own data, CPU work only -- safe on a
/// loader thread, any number at once: its ground from the game's DEM
/// (64 cells at the finest level, 32 coarser) with a skirt, its
/// land-class map, its landmarks, and its buildings when it is one of
/// the two finest levels. A tile with no data at all -- open ocean --
/// comes back as flat ground at sea level, never as a hole.
Fs2024PreparedTile PrepareFs2024Tile(Fs2024World& world,
                                     const Fs2024TileKey& key);

/// The tile's landmarks' bounds, decoding (into `tile.models`) every
/// model whose kit is not already on the GPU.
Fs2024LandmarkBoundsMap PrepareFs2024TileLandmarks(Fs2024World& world,
                                                   Fs2024PreparedTile& tile);

/// The tile's buildings, from every finest-level quad inside it, less
/// those standing under its landmarks.
Fs2024BuildingMeshCpu PrepareFs2024TileBuildings(
    Fs2024World& world, const Fs2024PreparedTile& tile,
    const Fs2024LandmarkBoundsMap& bounds);

}  // namespace sdl3cpp::services::impl
