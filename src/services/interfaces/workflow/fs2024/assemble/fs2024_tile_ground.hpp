#pragma once

#include "services/interfaces/workflow/fs2024/assemble/fs2024_class_sampler.hpp"
#include "services/interfaces/workflow/fs2024/assemble/fs2024_dem_sampler.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_heightfield.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One quad tile's ground heights from FS2024's own DEM -- tile
/// (quadX, quadY) at `level` -- as a `cells` x `cells` grid in the
/// tile's own local space: origin (0, 0) at its north-west corner,
/// `span` metres across, heights in metres above sea level.
/// Neighbouring tiles of one level sample the same Mercator points
/// along a shared edge, so their edges meet exactly.
Fs2024Heightfield BuildFs2024TileHeights(Fs2024DemSampler& dem, int quadX,
                                         int quadY, int level, float span,
                                         int cells);

/// The same tile's land classes, `size` x `size`, sampled at texel
/// centres: one byte each, FS2024's own class codes (see
/// Fs2024ClassSampler::ClassAt), row 0 the tile's north edge.
std::vector<std::uint8_t> BuildFs2024TileClasses(Fs2024ClassSampler& classes,
                                                 int quadX, int quadY,
                                                 int level, int size);

}  // namespace sdl3cpp::services::impl
