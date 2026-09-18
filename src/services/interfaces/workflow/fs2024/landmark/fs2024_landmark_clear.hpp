#pragma once

#include "services/interfaces/workflow/fs2024/assemble/fs2024_tile_buildings.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_instance.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_mesh.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

/// Every landmark model's bounds, by its own GXML name.
using Fs2024LandmarkBoundsMap =
    std::unordered_map<std::string, Fs2024LandmarkBounds>;

/// Drops every generated building whose centre stands inside a
/// landmark's own footprint -- its model-space bounds, turned the way
/// the landmark is -- so FS2024's imagery-derived outline of the Palace
/// of Westminster does not rise through the real model. Landmarks lower
/// than a storey (pontoons, docks, plazas) clear nothing: buildings
/// really do stand beside and on them.
void DropFs2024BuildingsUnderLandmarks(
    std::vector<Fs2024BuildingPlan>& plans,
    const std::vector<Fs2024LandmarkInstance>& instances,
    const Fs2024LandmarkBoundsMap& bounds);

}  // namespace sdl3cpp::services::impl
