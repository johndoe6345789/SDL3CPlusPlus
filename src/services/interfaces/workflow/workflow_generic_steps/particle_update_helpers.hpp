#pragma once

#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// particle.update's tunable parameters. gravity and enableFade are
/// parsed but currently unused by Execute -- preserved as-is.
struct ParticleUpdateParams {
    float deltaTime  = 0.016f;
    float gravity    = 9.81f;
    float damping    = 1.0f;
    bool enableFade  = false;
};

ParticleUpdateParams ReadParticleUpdateParams(
    const WorkflowStepDefinition& step, const WorkflowContext& context);

/// The result of aging and pruning particles.active/.ages/.lifetimes.
struct PrunedParticles {
    std::vector<std::string> particles;
    std::vector<float> ages;
    std::vector<float> lifetimes;
};

/// Ages every particle in `ages` by `deltaTime`, then drops any whose
/// age has reached its lifetime.
PrunedParticles AgeAndPruneParticles(const std::vector<std::string>& particles,
                                     std::vector<float> ages,
                                     std::vector<float> lifetimes,
                                     float deltaTime);

}  // namespace sdl3cpp::services::impl
