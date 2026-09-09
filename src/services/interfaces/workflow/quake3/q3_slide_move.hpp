#pragma once

#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"

namespace sdl3cpp::q3 {

/// Quake lets the player walk up anything this tall without jumping.
/// STEPSIZE in ioq3 bg_local.h is 18 Quake units.
inline constexpr float kStepSize = 18.0f / 32.0f;

/**
 * @brief Move the player, sliding along whatever it hits.
 *
 * Mirrors ioq3 bg_slidemove.c PM_SlideMove: trace, advance, clip the
 * velocity to the planes hit, repeat up to four bumps.
 *
 * @return true when the move was obstructed, matching PM_SlideMove's
 *         return value, which PM_StepSlideMove uses to decide whether a
 *         step up is worth attempting.
 */
bool SlideMove(services::impl::Q3PlayerState& ps,
               btDiscreteDynamicsWorld* world, float dt);

}  // namespace sdl3cpp::q3
