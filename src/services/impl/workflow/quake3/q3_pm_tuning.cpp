#include "services/interfaces/workflow/quake3/q3_pm_tuning.hpp"

namespace sdl3cpp::services::impl {
namespace {

float NumberOr(const WorkflowStepDefinition& step, const char* name,
               float fallback) {
    const auto it = step.parameters.find(name);
    if (it == step.parameters.end() ||
        it->second.type != WorkflowParameterValue::Type::Number) {
        return fallback;
    }
    return static_cast<float>(it->second.numberValue);
}

}  // namespace

q3::Q3PmTuning Q3TuningOf(const WorkflowStepDefinition& step) {
    q3::Q3PmTuning out;
    out.maxSpeed = NumberOr(step, "max_speed", out.maxSpeed);
    out.sprintSpeed = NumberOr(step, "sprint_speed", out.maxSpeed);
    out.stopSpeed = NumberOr(step, "stop_speed", out.stopSpeed);
    out.friction = NumberOr(step, "friction", out.friction);
    out.accelerate = NumberOr(step, "accelerate", out.accelerate);
    out.airAccelerate = NumberOr(step, "air_accelerate", out.airAccelerate);
    out.jumpVelocity = NumberOr(step, "jump_velocity", out.jumpVelocity);
    out.gravity = NumberOr(step, "gravity", out.gravity);
    return out;
}

}  // namespace sdl3cpp::services::impl
