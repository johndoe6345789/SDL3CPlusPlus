#pragma once

#include "services/interfaces/workflow/fs2024/building/fs2024_polygon.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_tile_writer.hpp"

#include <vector>

namespace sdl3cpp::fs2024 {

/// The mean of a footprint's own points.
sdl3cpp::services::impl::Point2 FootprintCentre(
    const std::vector<sdl3cpp::services::impl::Point2>& ring);

/// The building already standing where `centre` is, or nullptr. Two
/// records within a few metres of each other are the same real
/// building seen by both of FS2024's data sets.
BuildingFootprint* BuildingStandingHere(
    std::vector<BuildingFootprint>& existing,
    const sdl3cpp::services::impl::Point2& centre);

}  // namespace sdl3cpp::fs2024
