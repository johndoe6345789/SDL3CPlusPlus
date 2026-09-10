#pragma once

#include "services/interfaces/workflow/gta5/gta5_placement.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// Convert one entry of a tile file's "placements" array.
///
/// Returns false for an entry with no archetype name, which is the one
/// field a placement cannot be reconstructed without.
bool ParseGta5Placement(const nlohmann::json& entry,
                        Gta5Placement& outPlacement);

}  // namespace sdl3cpp::services::impl
