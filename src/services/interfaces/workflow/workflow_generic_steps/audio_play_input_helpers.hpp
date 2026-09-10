#pragma once

#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Reads audio.play's `mode` input/parameter, defaulting to "effect".
std::string ResolveAudioPlayMode(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver);

/// Reads audio.play's `loop` input/parameter, defaulting to `fallback`.
bool ResolveAudioPlayLoop(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver, bool fallback);

/// Lowercases `mode` for the background/music vs. effect/sfx comparison.
std::string NormalizeAudioPlayMode(std::string mode);

}  // namespace sdl3cpp::services::impl
