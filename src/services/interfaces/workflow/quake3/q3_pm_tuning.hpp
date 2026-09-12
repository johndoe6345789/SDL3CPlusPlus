#pragma once

#include "services/interfaces/workflow/quake3/q3_pm_constants.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

namespace sdl3cpp::q3 {

/// What the mover runs on, read per step.
///
/// Every default is Quake 3's own, so a workflow naming none of them
/// moves exactly as ioq3 does. GTA V names them all: a man walking
/// around Los Santos is not a marine who tops out at 10 m/s, stops
/// dead, and clears his own height from standing.
struct Q3PmTuning {
    float maxSpeed{kMaxSpeed};
    float sprintSpeed{kMaxSpeed};  // while input.sprint is held
    float stopSpeed{kStopSpeed};
    float friction{kFriction};
    float accelerate{kAccelerate};
    float airAccelerate{kAirAccelerate};
    float jumpVelocity{kJumpVelocity};
    float gravity{kGravity};
};

}  // namespace sdl3cpp::q3

namespace sdl3cpp::services::impl {

/// `step`'s movement numbers, each falling back to Quake's own.
q3::Q3PmTuning Q3TuningOf(const WorkflowStepDefinition& step);

}  // namespace sdl3cpp::services::impl
