#include "services/interfaces/workflow/quake3/workflow_q3_weapon_update_step.hpp"

// NOTE: This step has been superseded by the following dedicated steps:
//   q3.weapon.select  (workflow_q3_weapon_select_step)
//   q3.weapon.fire    (workflow_q3_weapon_fire_step)
// It is kept registered for backwards compatibility but performs no work.
// Remove it from workflow JSON files and use the new steps instead.

namespace sdl3cpp::services::impl {

WorkflowQ3WeaponUpdateStep::WorkflowQ3WeaponUpdateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3WeaponUpdateStep::GetPluginId() const {
    return "q3.weapon.update";
}

void WorkflowQ3WeaponUpdateStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& /*context*/) {
    // No-op: weapon selection and firing are now handled by
    // q3.weapon.select and q3.weapon.fire respectively.
}

}  // namespace sdl3cpp::services::impl
