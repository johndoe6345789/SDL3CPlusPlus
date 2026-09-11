#pragma once

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// How far ahead to stream: velocity_lead_seconds of horizontal travel.
/// Seated, the car's velocity -- the player body is pinned to it and reads
/// zero. Held short enough that the tiles it asks for stay inside the
/// evict radius, or evict would drop them again. Zero with prefetch off.
glm::vec3 Gta5StreamLead(const Gta5StreamState& state,
                         const glm::vec3& playerVelocity);

}  // namespace sdl3cpp::services::impl
