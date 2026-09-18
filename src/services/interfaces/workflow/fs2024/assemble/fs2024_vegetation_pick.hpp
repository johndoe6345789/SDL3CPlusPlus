#pragma once

#include "services/interfaces/workflow/fs2024/data/vegetation/fs2024_vegetation_library.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// The name of one of FS2024's own fallback biome rules to stand
/// vegetation on land class `landClass` (GlobeLand30's numbering: 2
/// forest, 4 shrubland) at `latitude` degrees. FS2024 itself picks a
/// rule by matching its own Potential Natural Vegetation and eco
/// region rasters; this stands in for that with a hand-picked mapping
/// from land class and climate band to one of FS2024's own named
/// fallback rules, so the real species mixes and densities are used,
/// just chosen more coarsely than the game's own biome match. Empty
/// for a land class this does not stand vegetation on (cropland,
/// grassland -- already textured as ground -- wetland, artificial,
/// bare).
std::string Fs2024VegetationBiomeFor(int landClass, double latitude);

}  // namespace sdl3cpp::services::impl
