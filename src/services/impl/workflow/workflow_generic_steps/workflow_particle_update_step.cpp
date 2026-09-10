#include "services/interfaces/workflow/workflow_generic_steps/workflow_particle_update_step.hpp"

#include "services/interfaces/workflow/workflow_generic_steps/particle_update_helpers.hpp"

#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowParticleUpdateStep::WorkflowParticleUpdateStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowParticleUpdateStep::GetPluginId() const {
    return "particle.update";
}

void WorkflowParticleUpdateStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    const ParticleUpdateParams params =
        ReadParticleUpdateParams(step, context);

    const auto* particlesPtr =
        context.TryGet<std::vector<std::string>>("particles.active");
    if (!particlesPtr || particlesPtr->empty()) {
        if (logger_) {
            logger_->Trace("WorkflowParticleUpdateStep", "Execute",
                           "No active particles",
                           "Particle update complete");
        }
        return;
    }
    const std::vector<std::string> particles = *particlesPtr;

    std::vector<float> ages;
    if (const auto* existing =
            context.TryGet<std::vector<float>>("particles.ages")) {
        ages = *existing;
    } else {
        ages.resize(particles.size(), 0.0f);
    }

    std::vector<float> lifetimes;
    if (const auto* existing =
            context.TryGet<std::vector<float>>("particles.lifetimes")) {
        lifetimes = *existing;
    } else {
        lifetimes.resize(particles.size(), 2.0f);
    }

    if (ages.size() == particles.size() &&
        lifetimes.size() == particles.size()) {
        const PrunedParticles pruned = AgeAndPruneParticles(
            particles, std::move(ages), std::move(lifetimes),
            params.deltaTime);
        context.Set("particles.active", pruned.particles);
        context.Set("particles.ages", pruned.ages);
        context.Set("particles.lifetimes", pruned.lifetimes);
    }

    if (logger_) {
        auto* finalParticles =
            context.TryGet<std::vector<std::string>>("particles.active");
        const size_t count = finalParticles ? finalParticles->size() : 0;
        logger_->Trace("WorkflowParticleUpdateStep", "Execute",
                       "delta=" + std::to_string(params.deltaTime) +
                           ", count=" + std::to_string(count),
                       "Particle update complete");
    }
}

}  // namespace sdl3cpp::services::impl
