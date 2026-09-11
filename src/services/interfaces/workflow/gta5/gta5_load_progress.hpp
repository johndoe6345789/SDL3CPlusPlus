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

/// Done once the tile under `at` has been read and every placement in it
/// spawned. Before that: the asset index is still building, then the
/// tile is being read, then its placements are streaming in -- each
/// reported as such, with a percentage for the last.
Gta5LoadProgress MeasureGta5LoadProgress(const Gta5StreamState& state,
                                         const glm::vec3& at);

}  // namespace sdl3cpp::services::impl
