#pragma once

/// Internal helpers shared by q3_step_slide_trace.cpp and q3_step_slide.cpp,
/// which together implement ApplyQ3StepSlideMove() from q3_step_slide.hpp.
/// Not part of the public workflow-step API.

#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"

#include <optional>

namespace sdl3cpp::q3::step_slide_detail {

/// Checks ground clearance and headroom for a step attempt starting at
/// `startOrigin`. Returns the up-trace (its endPos.y - startOrigin.y is
/// the candidate step height) or nullopt if the step must be rejected
/// outright: still rising with no ground below, or no headroom above.
std::optional<services::impl::Q3Trace> ComputeStepUpTrace(
    btDiscreteDynamicsWorld* world, const glm::vec3& startOrigin,
    const glm::vec3& startVelocity, const services::impl::Q3PlayerState& ps,
    const btCollisionObject* self);

/// Settles `stepped` (already raised by `stepSize` and slid) back down
/// onto whatever is underfoot. On success, updates `stepped.origin` to
/// the settled position and zeroes any remaining downward velocity, and
/// returns true. Returns false (leaving `stepped` unmodified) if the
/// settle could not land on walkable ground, in which case the step
/// attempt must be discarded.
bool SettleSteppedMove(btDiscreteDynamicsWorld* world,
                       services::impl::Q3PlayerState& stepped, float stepSize,
                       const btCollisionObject* self);

}  // namespace sdl3cpp::q3::step_slide_detail
