#include "services/interfaces/workflow/workflow_generic_steps/particle_update_helpers.hpp"

namespace sdl3cpp::services::impl {

ParticleUpdateParams ReadParticleUpdateParams(
    const WorkflowStepDefinition& step, const WorkflowContext& context) {
    WorkflowStepParameterResolver parameterResolver;
    ParticleUpdateParams out;

    if (const auto* param =
            parameterResolver.FindParameter(step, "delta_time")) {
        if (param->type == WorkflowParameterValue::Type::Number) {
            out.deltaTime = static_cast<float>(param->numberValue);
        }
    } else if (const auto* elapsed = context.TryGet<float>("frame.elapsed")) {
        out.deltaTime = *elapsed;
    }

    if (const auto* param = parameterResolver.FindParameter(step, "gravity")) {
        if (param->type == WorkflowParameterValue::Type::Number) {
            out.gravity = static_cast<float>(param->numberValue);
        }
    }

    if (const auto* param = parameterResolver.FindParameter(step, "damping")) {
        if (param->type == WorkflowParameterValue::Type::Number) {
            out.damping = static_cast<float>(param->numberValue);
            if (out.damping < 0.0f) out.damping = 0.0f;
            if (out.damping > 1.0f) out.damping = 1.0f;
        }
    }

    if (const auto* param =
            parameterResolver.FindParameter(step, "enable_fade")) {
        if (param->type == WorkflowParameterValue::Type::Bool) {
            out.enableFade = param->boolValue;
        }
    }

    return out;
}

PrunedParticles AgeAndPruneParticles(const std::vector<std::string>& particles,
                                     std::vector<float> ages,
                                     std::vector<float> lifetimes,
                                     float deltaTime) {
    // Precondition: ages.size() == lifetimes.size() == particles.size().
    // The caller is responsible for checking this first and skipping
    // the update entirely otherwise (matching the original step).
    for (auto& age : ages)
        age += deltaTime;

    PrunedParticles out;
    for (size_t i = 0; i < particles.size(); ++i) {
        if (ages[i] < lifetimes[i]) {
            out.particles.push_back(particles[i]);
            out.ages.push_back(ages[i]);
            out.lifetimes.push_back(lifetimes[i]);
        }
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
