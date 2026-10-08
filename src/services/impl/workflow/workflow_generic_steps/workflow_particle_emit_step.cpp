#include "services/interfaces/workflow/workflow_generic_steps/workflow_particle_emit_step.hpp"

#include "services/interfaces/workflow/executor/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow/particles/particle_pool.hpp"

#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

namespace {

float ReadNumber(const WorkflowStepDefinition& step, const std::string& key,
                 float fallback) {
    WorkflowStepParameterResolver parameterResolver;
    if (const auto* param = parameterResolver.FindParameter(step, key)) {
        if (param->type == WorkflowParameterValue::Type::Number) {
            return static_cast<float>(param->numberValue);
        }
    }
    return fallback;
}

glm::vec3 ReadVector(const WorkflowStepDefinition& step,
                     const std::string& prefix) {
    return glm::vec3(ReadNumber(step, prefix + "_x", 0.0f),
                     ReadNumber(step, prefix + "_y", 0.0f),
                     ReadNumber(step, prefix + "_z", 0.0f));
}

ParticleEmitSpec ReadEmitSpec(const WorkflowStepDefinition& step) {
    ParticleEmitSpec spec;
    spec.origin = ReadVector(step, "origin");
    spec.velocity = ReadVector(step, "velocity");
    spec.spread = ReadNumber(step, "spread", 0.0f);
    spec.minLifetime = ReadNumber(step, "lifetime_min", 1.0f);
    spec.maxLifetime = ReadNumber(step, "lifetime_max", spec.minLifetime);
    spec.size = ReadNumber(step, "size", 0.5f);
    spec.sizeGrowth = ReadNumber(step, "size_growth", 0.0f);
    spec.drag = ReadNumber(step, "drag", 0.0f);
    spec.gravityScale = ReadNumber(step, "gravity_scale", 1.0f);
    const float count = ReadNumber(step, "count", 10.0f);
    spec.count = count > 0.0f ? static_cast<std::uint32_t>(count) : 0u;
    return spec;
}

}  // namespace

WorkflowParticleEmitStep::WorkflowParticleEmitStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowParticleEmitStep::GetPluginId() const {
    return "particle.emit";
}

void WorkflowParticleEmitStep::Execute(const WorkflowStepDefinition& step,
                                       WorkflowContext& context) {
    const ParticleEmitSpec spec = ReadEmitSpec(step);

    ParticlePool pool;
    const auto* existing = context.TryGet<ParticlePool>("particles.pool");
    if (existing) {
        pool = *existing;
    }
    pool.Emit(spec);
    context.Set("particles.pool", pool);

    if (logger_) {
        logger_->Trace("WorkflowParticleEmitStep", "Execute",
                       "count=" + std::to_string(spec.count) +
                           ", live=" + std::to_string(pool.Count()),
                       "Emitted particles");
    }
}

}  // namespace sdl3cpp::services::impl
