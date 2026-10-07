#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>
#include <optional>
#include <string>

namespace sdl3cpp::services::impl {

/// With `state.library`, `state.track` and `state.racer` set: uploads the
/// track and pod models, builds the ground grid and the lap from the
/// spline, and puts the pod on the start line. False if the track has
/// no geometry or its spline has no closed lap.
bool BuildRacerWorld(SDL_GPUDevice* device, RacerWorldState& state,
                     const std::shared_ptr<ILogger>& logger);

/// The colour of a racer's energy binder.
glm::vec3 RacerBinderColour(const std::string& racer);

/// Samples the lap's Bezier curves into `state.lapPoints`.
void BuildRacerLapPoints(RacerWorldState& state);

/// Adds the track's collision triangles to `state.ground`. False when
/// the track has none (then its visible triangles are used instead).
bool BuildRacerCollisionGround(RacerWorldState& state,
                               const RacerModel& track,
                               const std::shared_ptr<ILogger>& logger);

/// The colour along the bottom of the sky band (its textures and vertex
/// colours averaged), so distance fog and the horizon match the sky.
/// Empty when the track has no sky.
std::optional<glm::vec3> RacerSkyHorizonColour(
    const RacerAssetLibrary& library, const RacerModel& sky);

/// Trace: how wide the rideable floor is across the lap (median and
/// narrowest, in metres), for checking pods against the track's scale.
void TraceRacerTrackWidth(const std::shared_ptr<ILogger>& logger,
                          const RacerWorldState& state);

/// Trace: a model's extent in engine units, for checking scale.
void TraceModelExtent(const std::shared_ptr<ILogger>& logger,
                      const std::string& what, const RacerModel& model);

/// Gives a pod its body for contact (from its rig) and its bump mass.
void SetRacerPodBody(RacerPodState& pod, const RacerPodRig& rig,
                     const RacerPodSpec& spec);

/// Puts `pod` on lap point `index` facing the next one, `lateral`
/// metres to its right, at rest height above the surface there.
void PlaceRacerPod(const RacerWorldState& state, RacerPodState& pod,
                   int index, float speed, float lateral);

/// Puts the player's pod on lap point `index`, facing the next one, at rest
/// height above whatever surface is there.
void PlaceRacerPodOnLap(RacerWorldState& state, int index, float speed);

}  // namespace sdl3cpp::services::impl
