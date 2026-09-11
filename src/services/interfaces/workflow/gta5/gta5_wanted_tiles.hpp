#pragma once

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Fill state.wanted with the tiles within the load radius of `origin`,
/// where the player is, or of `lead`, where they are heading -- so tiles
/// ahead are read before they are reached. Under the resident budget the
/// nearest to either win.
void ResolveGta5WantedTiles(Gta5StreamState& state, const glm::vec3& origin,
                            const glm::vec3& lead);

}  // namespace sdl3cpp::services::impl
