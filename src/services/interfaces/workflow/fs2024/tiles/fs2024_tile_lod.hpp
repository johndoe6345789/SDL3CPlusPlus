#pragma once

#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_key.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

/// How far each level reaches.
struct Fs2024LodConfig {
    /// Coarsest tiles shown on each side of the viewer's own.
    int rootRadius = 2;
    /// A tile splits into its four children while the viewer is closer
    /// to it than `split` times its width: at 1, level 14 reaches about
    /// one level-13 width (3 km at London), 13 twice that, and so on.
    float split = 1.f;
};

/// The tiles to show a viewer at `viewer` -- engine x, z, and y its
/// height above the ground, so climbing coarsens everything below --
/// cut from a square of coarsest tiles as a quadtree: each tile close
/// enough splits, down to the finest level. The leaves cover the square
/// exactly once, nearest first. `tileSize` is the finest level's width.
std::vector<Fs2024TileKey> SelectFs2024Tiles(const glm::vec3& viewer,
                                             float tileSize,
                                             const Fs2024LodConfig& config);

/// Horizontal distance from (x, z) to the tile (0 inside it), combined
/// with the viewer's height.
float Fs2024TileDistance(const Fs2024TileKey& key, float tileSize,
                         const glm::vec3& viewer);

}  // namespace sdl3cpp::services::impl
