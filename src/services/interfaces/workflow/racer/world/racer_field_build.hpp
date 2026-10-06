#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/// Fills the grid: up to `count` opponents from the racer table (never
/// the player's racer), each with their own pod model and a pace
/// between 0.9 and 1.0 of a stock pod, then lines everyone up in rows
/// of two from the start line with the player in the second row.
void BuildRacerField(SDL_GPUDevice* device, RacerWorldState& state,
                     const RacerTrackTable& table, int count,
                     const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
