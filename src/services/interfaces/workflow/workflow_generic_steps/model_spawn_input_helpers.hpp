#pragma once

#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <array>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Reads model.spawn's `matrix` input/parameter (16 numbers, row-major),
/// defaulting to identity if neither is set.
std::array<float, 16> ReadMatrix(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver);

/// Reads model.spawn's shader keys from (in priority order) the
/// `shader_keys` input, `shader_key` input, `shader_keys` parameter, or
/// `shader_key` parameter. Throws if none are set.
std::vector<std::string> ReadShaderKeys(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver);

/// Reads model.spawn's `object_type` input/parameter, defaulting to "".
std::string ReadObjectType(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver);

}  // namespace sdl3cpp::services::impl
