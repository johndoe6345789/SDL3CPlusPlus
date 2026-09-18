#pragma once

#include "services/interfaces/workflow/bl4/bl4_tile_stream_state.hpp"

#include <glm/glm.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/// How much of the ring around `at` is in: `done` once no tile within
/// the load radius is still queued, and a short line for the overlay
/// (20 characters of SDL's debug font) meanwhile.
struct Bl4LoadProgress {
    bool done = false;
    std::string text;
};

Bl4LoadProgress MeasureBl4LoadProgress(const Bl4TileStreamState& state,
                                       const glm::vec3& at);

}  // namespace sdl3cpp::services::impl
