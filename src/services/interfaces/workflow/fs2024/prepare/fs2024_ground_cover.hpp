#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace sdl3cpp::tools::fs2024 {

/// Land cover guessed from height and slope, in the absence of real
/// imagery: meadow low down, forest where it steepens, rock and snow
/// higher still. A stand-in that reads right for hilly terrain; flat
/// ground (a city) just comes out a uniform meadow green, which is
/// fine since a road/building bake's ground rarely shows much of it.
std::array<std::uint8_t, 3> CoverColour(float altitude, float slopeDegrees);

/// Per-point slope in degrees from a height grid at `spacing` metres,
/// smoothed first (a 30 m DSM's metre-scale noise -- roofs, tree
/// crowns -- otherwise speckles the cover classes into stripes).
std::vector<float> ComputeSlopeDegrees(const std::vector<float>& heights,
                                       int cells, float spacing);

/// Nudges `heights` towards `target` wherever `mask` (0..255, `cells`
/// x `cells`) is set, growing the mask a little and blurring the
/// transition first so pavement does not end in a cliff.
void FlattenTowards(std::vector<float>& heights, int cells,
                   const std::vector<std::uint8_t>& mask, float target);

}  // namespace sdl3cpp::tools::fs2024
