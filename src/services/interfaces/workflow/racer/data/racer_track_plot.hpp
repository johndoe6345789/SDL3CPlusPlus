#pragma once

#include "services/interfaces/workflow/racer/data/racer_spline.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// A top-down RGBA picture of a spline's three point rows, each drawn in
/// its own colour, scaled to fit a square of `size` pixels. Used to check
/// that a spline reads as a track before anything renders it.
std::vector<std::uint8_t> PlotRacerTrack(
    const std::vector<RacerSplineRecord>& records, int size);

}  // namespace sdl3cpp::services::impl
