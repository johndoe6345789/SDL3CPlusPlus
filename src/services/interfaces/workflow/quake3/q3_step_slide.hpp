#pragma once

#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"

#include <optional>

namespace sdl3cpp::q3 {

/**
 * @brief Slides `ps` and, if that move was obstructed while walking on
 * the ground, retries it one step height higher so stairs, ledges and
 * doorway lips are walked over rather than blocking.
 *
 * Mirrors ioq3 bg_slidemove.c PM_StepSlideMove. Mutates `ps` to the
 * final chosen result (the plain slide, or the stepped-and-settled
 * move if that turned out to land on walkable ground).
 *
 * @return The vertical rise applied by a successful step; 0 if the
 *         plain slide reached its target first try (no step needed);
 *         or nullopt if a step was attempted but rejected (no headroom,
 *         no ground below, or the settle landed somewhere unwalkable) —
 *         callers should leave q3.step_delta at its previous value in
 *         that case, matching the original monolithic step.
 */
std::optional<float> ApplyQ3StepSlideMove(services::impl::Q3PlayerState& ps,
                                          btDiscreteDynamicsWorld* world,
                                          float dt,
                                          const btCollisionObject* self);

}  // namespace sdl3cpp::q3
