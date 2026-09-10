#include "services/interfaces/workflow/workflow_generic_steps/vfx_destroy_selection.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/vfx_destroy_all_or_target.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/vfx_destroy_by_id.hpp"

namespace sdl3cpp::services::impl {

bool ApplyVfxDestroySelection(const WorkflowStepDefinition& step,
                              const WorkflowStepParameterResolver& resolver,
                              std::vector<std::string>& effects) {
    if (DestroyAll(step, resolver, effects)) return true;
    if (DestroyById(step, resolver, effects)) return true;
    if (DestroyByCommaSeparatedIds(step, resolver, effects)) return true;
    return DestroyByTarget(step, resolver, effects);
}

}  // namespace sdl3cpp::services::impl
