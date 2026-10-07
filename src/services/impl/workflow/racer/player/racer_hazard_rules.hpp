#pragma once

#include "services/interfaces/workflow/racer/player/racer_hazards.hpp"

namespace sdl3cpp::services::impl {

inline constexpr float kRacerEruptSeconds = 1.8f;    // of each cycle
inline constexpr float kRacerRockFallSeconds = 1.6f;

/// A Tusken fires at the nearest pod in range; some shots hit an
/// engine. False when no pod is in range.
bool FireRacerBlaster(RacerHazard& hazard,
                      const std::vector<RacerPodState*>& pods);

/// An erupting vent heats and lifts the pods over it.
void RunRacerEruption(const RacerHazard& hazard,
                      const std::vector<RacerPodState*>& pods, float dt);

/// A landing rock batters and slows the pods under it.
void DropRacerRock(const RacerHazard& hazard,
                   const std::vector<RacerPodState*>& pods);

}  // namespace sdl3cpp::services::impl
