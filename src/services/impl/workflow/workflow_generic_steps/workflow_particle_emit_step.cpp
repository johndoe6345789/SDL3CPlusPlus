#include "services/interfaces/workflow/workflow_generic_steps/workflow_particle_emit_step.hpp"

#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <utility>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowParticleEmitStep::WorkflowParticleEmitStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowParticleEmitStep::GetPluginId() const {
    return "particle.emit";
}

void WorkflowParticleEmitStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver parameterResolver;
    WorkflowStepIoResolver ioResolver;

    // Get emitter_id (optional)
    std::string emitterId = "emitter_default";
    if (const auto* param = parameterResolver.FindParameter(step, "emitter_id")) {
        if (param->type == WorkflowParameterValue::Type::String) {
            emitterId = param->stringValue;
        }
    }

    // Get particle count
    int count = 10;
    if (const auto* param = parameterResolver.FindParameter(step, "count")) {
        if (param->type == WorkflowParameterValue::Type::Number) {
            count = static_cast<int>(param->numberValue);
            if (count < 0) count = 0;
        }
    }

    // Log particle emission
    if (logger_) {
        logger_->Trace("WorkflowParticleEmitStep", "Execute",
                       "emitter=" + emitterId + ", count=" + std::to_string(count),
                       "Emitting particles");
    }

    // Initialize or get particles vector
    std::vector<std::string> particles;
    if (const auto* existingParticles = context.TryGet<std::vector<std::string>>("particles.active")) {
        particles = *existingParticles;
    }

    // Add particles
    for (int i = 0; i < count; ++i) {
        particles.push_back(emitterId + "_p" + std::to_string(particles.size()));
    }

    // Store back in context
    context.Set("particles.active", particles);
}

}  // namespace sdl3cpp::services::impl
