#pragma once

#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// vfx.destroy's `vfx_id` selector: removes one exact-match effect id.
bool DestroyById(const WorkflowStepDefinition& step,
                 const WorkflowStepParameterResolver& resolver,
                 std::vector<std::string>& effects);

/// vfx.destroy's `vfx_ids` selector: removes every id in a comma-separated
/// list (each entry leading-whitespace-trimmed) that matches an effect.
bool DestroyByCommaSeparatedIds(const WorkflowStepDefinition& step,
                                const WorkflowStepParameterResolver& resolver,
                                std::vector<std::string>& effects);

}  // namespace sdl3cpp::services::impl
