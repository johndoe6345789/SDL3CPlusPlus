#pragma once

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <glm/glm.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/// How far the ground under a point is from being there to stand on.
struct Gta5LoadProgress {
    bool done{false};
    /// What to tell the player while waiting; empty once done.
    std::string text;
};

/// Done once the tile under `at` and its wanted neighbours have been read
/// and every placement in them spawned -- neighbours too, because a
/// terrain piece tiled by its origin can cover a point in the next tile.
/// Before that: the index is building, then tiles are being read, then
/// placements are streaming in, with a percentage over all nine.
Gta5LoadProgress MeasureGta5LoadProgress(const Gta5StreamState& state,
                                         const glm::vec3& at);

}  // namespace sdl3cpp::services::impl
