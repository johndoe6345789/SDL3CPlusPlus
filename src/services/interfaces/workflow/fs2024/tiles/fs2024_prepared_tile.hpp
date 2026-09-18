#pragma once

#include "services/interfaces/workflow/fs2024/assemble/fs2024_tile_building_mesh.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_instance.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_mesh.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_mesh.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_key.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// One tile built from FS2024's own data on a loader thread, everything
/// but the GPU and Bullet work the main thread finishes it with.
struct Fs2024PreparedTile {
    Fs2024TileKey key;
    std::string error;  ///< why it could not be built; empty when it was
    Fs2024Heightfield field;  ///< in the tile's own space
    Fs2024TerrainChunkMesh ground;
    std::vector<std::uint8_t> classes;  ///< classSize x classSize
    int classSize = 0;
    Fs2024BuildingMeshCpu buildings;
    std::vector<Fs2024LandmarkInstance> landmarks;
    /// Models this tile stands that were not on the GPU when it was
    /// built, decoded and ready to upload.
    std::vector<std::shared_ptr<const Fs2024LandmarkMesh>> models;
};

}  // namespace sdl3cpp::services::impl
