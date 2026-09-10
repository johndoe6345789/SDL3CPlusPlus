#include "services/interfaces/workflow/workflow_generic_steps/physics_fps_input.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {
namespace {

float NumberParameter(const WorkflowStepParameterResolver& params,
                      const WorkflowStepDefinition& step, const char* key,
                      float fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isNumber = p && p->type == WorkflowParameterValue::Type::Number;
    return isNumber ? static_cast<float>(p->numberValue) : fallback;
}

}  // namespace

FpsMoveKeys ReadFpsMoveKeys(const WorkflowContext& context) {
    FpsMoveKeys keys;
    keys.forward = context.GetBool("input_key_w", false);
    keys.left    = context.GetBool("input_key_a", false);
    keys.back    = context.GetBool("input_key_s", false);
    keys.right   = context.GetBool("input_key_d", false);
    keys.jump    = context.GetBool("input_key_space", false);
    keys.sprint  = context.GetBool("input_key_shift", false);
    keys.crouch  = context.GetBool("input_key_ctrl", false);
    return keys;
}

FpsMoveParams ReadFpsMoveParams(const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    FpsMoveParams p;
    p.moveSpeed = NumberParameter(params, step, "move_speed", p.moveSpeed);
    p.sprintMultiplier =
        NumberParameter(params, step, "sprint_multiplier", p.sprintMultiplier);
    p.crouchMultiplier =
        NumberParameter(params, step, "crouch_multiplier", p.crouchMultiplier);
    p.crouchHeight =
        NumberParameter(params, step, "crouch_height", p.crouchHeight);
    p.standHeight =
        NumberParameter(params, step, "stand_height", p.standHeight);
    p.airControl = NumberParameter(params, step, "air_control", p.airControl);
    p.gravityScale =
        NumberParameter(params, step, "gravity_scale", p.gravityScale);
    p.groundAccel =
        NumberParameter(params, step, "ground_accel", p.groundAccel);
    p.groundFriction =
        NumberParameter(params, step, "ground_friction", p.groundFriction);
    p.stepHeight = NumberParameter(params, step, "step_height", p.stepHeight);
    p.jumpVelocity =
        NumberParameter(params, step, "jump_velocity", p.jumpVelocity);
    return p;
}

}  // namespace sdl3cpp::services::impl
