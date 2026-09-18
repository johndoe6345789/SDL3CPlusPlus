#pragma once

#include "services/interfaces/workflow/fs2024/assemble/fs2024_vec_shapes.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_mesh.hpp"


namespace sdl3cpp::services::impl {

/// Open water the vector layer does not outline: the sea off every coast,
/// and a far lake. FS2024's DEM holds open water at one constant surface,
/// where even flat land varies, so every grid cell of `field` whose
/// corners agree within 2 cm becomes water at that surface -- the sea,
/// Lake Geneva's 411 m, the Dead Sea's -430 m -- appended to `water`,
/// and the ground under it is lowered below. Cells whose ground the
/// vector layer maps (`shapes`) are left to its own water outlines.
/// (FS2024's own WaterClassification and LandClassification rasters
/// say what kind of water a place would have, not where it is.)
void AddFs2024OpenWater(const Fs2024TileShapes& shapes,
                        Fs2024Heightfield& field,
                        Fs2024TerrainChunkMesh& water);

}  // namespace sdl3cpp::services::impl
