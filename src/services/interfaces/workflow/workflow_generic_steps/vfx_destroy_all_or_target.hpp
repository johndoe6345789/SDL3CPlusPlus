#pragma once

#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// vfx.destroy's `destroy_all` selector: clears `effects` if the boolean
/// parameter is present and true.
bool DestroyAll(const WorkflowStepDefinition& step,
                const WorkflowStepParameterResolver& resolver,
                std::vector<std::string>& effects);

/// vfx.destroy's `target` selector ("oldest"/"newest"): removes one
/// effect from the corresponding end of `effects`, if any remain.
bool DestroyByTarget(const WorkflowStepDefinition& step,
                     const WorkflowStepParameterResolver& resolver,
                     std::vector<std::string>& effects);

}  // namespace sdl3cpp::services::impl
