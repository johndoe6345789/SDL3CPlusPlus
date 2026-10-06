#pragma once

#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// How far a pod has got: laps done times the lap length in points,
/// plus its point on the current lap. Finishers rank by finish time.
float RacerRaceProgress(const RacerRaceState& race, int lapPointCount);

/// Places every pod in the race (1 = leading) and records it in each
/// pod's race state.
void RankRacerField(RacerWorldState& state);

/// Pushes overlapping pods apart (they are about 5 m wide) and trades a
/// little speed between them, so pods jostle rather than pass through.
void SeparateRacerPods(const std::vector<RacerPodState*>& pods);

}  // namespace sdl3cpp::services::impl
