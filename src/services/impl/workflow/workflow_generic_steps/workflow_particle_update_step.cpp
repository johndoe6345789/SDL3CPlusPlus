#include "services/interfaces/workflow/workflow_generic_steps/workflow_particle_update_step.hpp"

#include "services/interfaces/workflow/executor/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow/particles/particle_pool.hpp"

#include <glm/glm.hpp>

#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

namespace {

float ReadDeltaTime(const WorkflowStepDefinition& step,
                    const WorkflowContext& context) {
    WorkflowStepParameterResolver parameterResolver;
    if (const auto* param =
            parameterResolver.FindParameter(step, "delta_time")) {
        if (param->type == WorkflowParameterValue::Type::Number) {
            return static_cast<float>(param->numberValue);
        }
    }
    if (const auto* elapsed = context.TryGet<float>("frame.elapsed")) {
        return *elapsed;
    }
    return 0.016f;
}

float ReadGravity(const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver parameterResolver;
    if (const auto* param = parameterResolver.FindParameter(step, "gravity")) {
        if (param->type == WorkflowParameterValue::Type::Number) {
            return static_cast<float>(param->numberValue);
        }
    }
    return 9.81f;
}

}  // namespace

WorkflowParticleUpdateStep::WorkflowParticleUpdateStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowParticleUpdateStep::GetPluginId() const {
    return "particle.update";
}

void WorkflowParticleUpdateStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    const auto* existing = context.TryGet<ParticlePool>("particles.pool");
    if (!existing) {
        if (logger_) {
            logger_->Trace("WorkflowParticleUpdateStep", "Execute",
                           "No particle pool", "Particle update complete");
        }
        return;
    }

    const float deltaTime = ReadDeltaTime(step, context);
    ParticlePool pool = *existing;
    pool.Step(deltaTime, glm::vec3(0.0f, -ReadGravity(step), 0.0f));
    context.Set("particles.pool", pool);

    if (logger_) {
        logger_->Trace("WorkflowParticleUpdateStep", "Execute",
                       "delta=" + std::to_string(deltaTime) +
                           ", count=" + std::to_string(pool.Count()),
                       "Particle update complete");
    }
}

}  // namespace sdl3cpp::services::impl
