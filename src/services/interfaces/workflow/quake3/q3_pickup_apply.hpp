#pragma once

#include "services/interfaces/workflow/quake3/q3_pickup_effects.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// True if `s` begins with `prefix`.
bool HasClassPrefix(const std::string& s, const std::string& prefix);

/// Applies one pickup's effect to `state`, returning its respawn delay
/// in seconds (matching ioq3 defaults).
double ApplyOnePickup(const std::string& cls, PickupTouchState& state);

}  // namespace sdl3cpp::services::impl
