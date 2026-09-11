#include "services/interfaces/workflow/gta5/gta5_vehicles_sync_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_vehicle.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5VehiclesSyncStep::WorkflowGta5VehiclesSyncStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5VehiclesSyncStep::GetPluginId() const {
    return "gta5.vehicles.sync";
}

void WorkflowGta5VehiclesSyncStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    if (!state_ || state_->vehicles.empty()) return;
    UpdateGta5Vehicles(*state_);
    context.Set("gta5.vehicles.count",
                static_cast<int>(state_->vehicles.size()));
}

}  // namespace sdl3cpp::services::impl
