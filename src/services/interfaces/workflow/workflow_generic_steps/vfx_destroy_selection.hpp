#pragma once

#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Applies vfx.destroy's selection parameters to `effects` in
 * place, in priority order: destroy_all, then vfx_id, then vfx_ids
 * (comma-separated, each leading-whitespace-trimmed), then target
 * ("oldest"/"newest") if nothing else destroyed and effects remain.
 * @return true if any effect (or all of them) was destroyed.
 */
bool ApplyVfxDestroySelection(const WorkflowStepDefinition& step,
    const WorkflowStepParameterResolver& resolver,
    std::vector<std::string>& effects);

}  // namespace sdl3cpp::services::impl
