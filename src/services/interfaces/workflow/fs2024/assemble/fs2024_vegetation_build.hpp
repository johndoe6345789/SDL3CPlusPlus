#pragma once

#include "services/interfaces/workflow/fs2024/data/vegetation/fs2024_vegetation_library.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_mesh.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// One species' worth of vegetation for a tile: every instance's
/// crossed-quad billboards, in tile-local metres, sharing the species'
/// own albedo texture array (its variations' own layers, and each
/// billboard's own view frame, are baked into the mesh already -- see
/// fs2024_vegetation_build.cpp).
struct Fs2024VegetationGroupCpu {
    std::string speciesName;
    std::string albedoPath;
    Fs2024TerrainChunkMesh mesh;
};

/// Stands FS2024's own trees and shrubs on a tile's forest and
/// shrubland, from the game's own species sizes, imposter atlases and
/// per-hectare densities -- picked by land class and latitude rather
/// than FS2024's own eco region/PNV rasters (see
/// fs2024_vegetation_pick.hpp), and thinned to a fixed budget rather
/// than the game's own (much denser) real spacing, so one tile's worth
/// stays cheap to build and draw. `classes` is the tile's land-class
/// raster (`classSize` square, row 0 north, as BuildFs2024TileClasses
/// returns it); `field` is its ground, already carved for water.
std::vector<Fs2024VegetationGroupCpu> BuildFs2024Vegetation(
    const sdl3cpp::fs2024::VegetationLibrary& library,
    const std::vector<std::uint8_t>& classes, int classSize,
    const Fs2024Heightfield& field, float tileSize, double centreLatitude,
    std::uint64_t tileSeed);

}  // namespace sdl3cpp::services::impl
