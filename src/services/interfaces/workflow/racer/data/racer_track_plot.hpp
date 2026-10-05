#pragma once

#include "services/interfaces/workflow/racer/data/racer_spline.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// A top-down RGBA picture of a spline, `size` pixels square: the main
/// lap in white and every other link (forks and alternate routes) in
/// orange, each drawn along its Bezier curve over the x/y ground plane.
std::vector<std::uint8_t> PlotRacerTrack(
    const std::vector<RacerSplineSegment>& segments, int size);

}  // namespace sdl3cpp::services::impl
