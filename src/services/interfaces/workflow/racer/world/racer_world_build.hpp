#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// With `state.library`, `state.track` and `state.racer` set: uploads the
/// track and pod models, builds the ground grid and the lap from the
/// spline, and puts the pod on the start line. False if the track has
/// no geometry or its spline has no closed lap.
bool BuildRacerWorld(SDL_GPUDevice* device, RacerWorldState& state,
                     const std::shared_ptr<ILogger>& logger);

/// Samples the lap's Bezier curves into `state.lapPoints`.
void BuildRacerLapPoints(RacerWorldState& state);

/// Adds the track's collision triangles to `state.ground`. False when
/// the track has none (then its visible triangles are used instead).
bool BuildRacerCollisionGround(RacerWorldState& state,
                               const RacerModel& track);

/// Trace: a model's extent in engine units, for checking scale.
void TraceModelExtent(const std::shared_ptr<ILogger>& logger,
                      const std::string& what, const RacerModel& model);

/// Puts the pod on lap point `index`, facing the next one, at rest
/// height above whatever surface is there.
void PlaceRacerPodOnLap(RacerWorldState& state, int index, float speed);

}  // namespace sdl3cpp::services::impl
