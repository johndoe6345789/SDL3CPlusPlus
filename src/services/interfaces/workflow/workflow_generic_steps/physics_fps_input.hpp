#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

namespace sdl3cpp::services::impl {

/// Held movement keys for one frame, read from input.poll's context keys.
struct FpsMoveKeys {
    bool forward = false, back = false, left = false, right = false;
    bool jump = false, sprint = false, crouch = false;
};

/// Reads input_key_{w,a,s,d,space,shift,ctrl} (set by input.poll).
FpsMoveKeys ReadFpsMoveKeys(const WorkflowContext& context);

/// The step's tunable parameters, each with the original plugin's default.
struct FpsMoveParams {
    float moveSpeed        = 6.0f;
    float sprintMultiplier = 1.8f;
    float crouchMultiplier = 0.4f;
    float crouchHeight     = 0.8f;
    float standHeight      = 1.6f;
    float airControl       = 0.3f;
    float gravityScale     = 1.0f;
    float groundAccel      = 35.0f;
    float groundFriction   = 30.0f;
    float stepHeight       = 0.55f;
    float jumpVelocity     = 6.5f;
};

FpsMoveParams ReadFpsMoveParams(const WorkflowStepDefinition& step);

}  // namespace sdl3cpp::services::impl
