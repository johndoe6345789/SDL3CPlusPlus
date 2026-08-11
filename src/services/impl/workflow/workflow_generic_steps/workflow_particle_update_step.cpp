#include "services/interfaces/workflow/workflow_generic_steps/workflow_particle_update_step.hpp"

#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <utility>
#include <string>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

WorkflowParticleUpdateStep::WorkflowParticleUpdateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowParticleUpdateStep::GetPluginId() const {
    return "particle.update";
}

void WorkflowParticleUpdateStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver parameterResolver;
    WorkflowStepIoResolver ioResolver;

    // Get delta time
    float deltaTime = 0.016f;
    if (const auto* param = parameterResolver.FindParameter(step, "delta_time")) {
        if (param->type == WorkflowParameterValue::Type::Number) {
            deltaTime = static_cast<float>(param->numberValue);
        }
    } else if (const auto* elapsed = context.TryGet<float>("frame.elapsed")) {
        deltaTime = *elapsed;
    }

    // Get gravity
    float gravity = 9.81f;
    if (const auto* param = parameterResolver.FindParameter(step, "gravity")) {
        if (param->type == WorkflowParameterValue::Type::Number) {
            gravity = static_cast<float>(param->numberValue);
        }
    }

    // Get damping
    float damping = 1.0f;
    if (const auto* param = parameterResolver.FindParameter(step, "damping")) {
        if (param->type == WorkflowParameterValue::Type::Number) {
            damping = static_cast<float>(param->numberValue);
            if (damping < 0.0f) damping = 0.0f;
            if (damping > 1.0f) damping = 1.0f;
        }
    }

    // Get enable_fade flag
    bool enableFade = false;
    if (const auto* param = parameterResolver.FindParameter(step, "enable_fade")) {
        if (param->type == WorkflowParameterValue::Type::Bool) {
            enableFade = param->boolValue;
        }
    }

    // Get active particles
    const auto* particlesPtr = context.TryGet<std::vector<std::string>>("particles.active");
    if (!particlesPtr || particlesPtr->empty()) {
        if (logger_) {
            logger_->Trace("WorkflowParticleUpdateStep", "Execute",
                           "No active particles",
                           "Particle update complete");
        }
        return;
    }

    std::vector<std::string> particles = *particlesPtr;

    // Get or initialize arrays
    std::vector<float> ages;
    if (const auto* existingAges = context.TryGet<std::vector<float>>("particles.ages")) {
        ages = *existingAges;
    } else {
        ages.resize(particles.size(), 0.0f);
    }

    std::vector<float> lifetimes;
    if (const auto* existingLifetimes = context.TryGet<std::vector<float>>("particles.lifetimes")) {
        lifetimes = *existingLifetimes;
    } else {
        lifetimes.resize(particles.size(), 2.0f);
    }

    if (ages.size() == particles.size() && lifetimes.size() == particles.size()) {
        // Age particles
        for (size_t i = 0; i < ages.size(); ++i) {
            ages[i] += deltaTime;
        }

        // Remove expired particles
        std::vector<std::string> newParticles;
        std::vector<float> newAges;
        std::vector<float> newLifetimes;

        for (size_t i = 0; i < particles.size(); ++i) {
            if (ages[i] < lifetimes[i]) {
                newParticles.push_back(particles[i]);
                newAges.push_back(ages[i]);
                newLifetimes.push_back(lifetimes[i]);
            }
        }

        context.Set("particles.active", newParticles);
        context.Set("particles.ages", newAges);
        context.Set("particles.lifetimes", newLifetimes);
    }

    if (logger_) {
        auto* finalParticles = context.TryGet<std::vector<std::string>>("particles.active");
        size_t count = finalParticles ? finalParticles->size() : 0;
        logger_->Trace("WorkflowParticleUpdateStep", "Execute",
                       "delta=" + std::to_string(deltaTime) + ", count=" + std::to_string(count),
                       "Particle update complete");
    }
}

}  // namespace sdl3cpp::services::impl
