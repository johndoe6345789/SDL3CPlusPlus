#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/// With `state.library`, `state.track` and `state.racer` set: uploads the
/// track and pod models, builds the ground grid and the lap from the
/// spline, and puts the pod on the start line. False if the track has
/// no geometry or its spline has no closed lap.
bool BuildRacerWorld(SDL_GPUDevice* device, RacerWorldState& state,
                     const std::shared_ptr<ILogger>& logger);

/// Puts the pod on lap point `index`, facing the next one, at rest
/// height above whatever surface is there.
void PlaceRacerPodOnLap(RacerWorldState& state, int index, float speed);

}  // namespace sdl3cpp::services::impl
