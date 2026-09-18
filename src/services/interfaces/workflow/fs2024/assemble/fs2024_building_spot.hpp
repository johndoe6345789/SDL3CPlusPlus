#pragma once

#include "services/interfaces/workflow/fs2024/assemble/fs2024_tile_buildings.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// The mean of a footprint's own points.
Point2 Fs2024FootprintCentre(const std::vector<Point2>& ring);

/// The plan already standing where `centre` is, or nullptr. Two records
/// within a few metres of each other are one real building seen by
/// both of FS2024's data sets.
Fs2024BuildingPlan* Fs2024PlanStandingAt(std::vector<Fs2024BuildingPlan>& plans,
                                         const Point2& centre);

}  // namespace sdl3cpp::services::impl
