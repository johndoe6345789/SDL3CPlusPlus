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

/// How many tiles further out to stream from `height` metres up, so a
/// vantage point sees the map to the horizon; far tiles bring only
/// their long-range LODs. It grows a ring at a time, once the tiles
/// wanted now are nearly all in -- the ground underfoot before the
/// horizon -- and shrinks with half a step of slack.
int Gta5VantageTiles(const Gta5StreamState& state, float height);

}  // namespace sdl3cpp::services::impl
