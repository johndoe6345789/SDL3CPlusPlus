#pragma once

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Fill state.wanted with the tiles that should be resident around
/// `centre`, capped at the configured maximum with the nearest winning.
void ResolveGta5WantedTiles(Gta5StreamState& state, const glm::vec3& centre);

}  // namespace sdl3cpp::services::impl
