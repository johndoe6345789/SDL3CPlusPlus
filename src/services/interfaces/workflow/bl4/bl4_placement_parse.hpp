#pragma once

#include "services/interfaces/workflow/bl4/bl4_placement.hpp"

#include <nlohmann/json.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

/// Parses one placement entry from a tile's placements.json. False (and
/// outPlacement untouched) when "archetype" is missing or empty.
bool ParseBl4Placement(const nlohmann::json& entry, Bl4Placement& outPlacement);

/// Parses every entry in a tile's placements.json "placements" array.
std::vector<Bl4Placement> ParseBl4TilePlacements(const nlohmann::json& doc);

}  // namespace sdl3cpp::services::impl
