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

    // Report once: a wheel drawn at the wrong place looks identical to
    // one that is not drawn at all.
    if (!reported_ && logger_) {
        reported_ = true;
        const Gta5Vehicle& car = state_->vehicles.front();
        const btVector3 body = car.chassis->getWorldTransform().getOrigin();
        std::string line = "gta5.vehicles.sync: chassis (" +
                           std::to_string(body.x()) + "," +
                           std::to_string(body.y()) + "," +
                           std::to_string(body.z()) + ") wheels";
        for (int i = 0; i < 4 && car.hasWheels; ++i) {
            const btVector3 w =
                car.vehicle->getWheelTransformWS(i).getOrigin();
            line += " [" + std::to_string(w.x()) + "," +
                    std::to_string(w.y()) + "," + std::to_string(w.z()) + "]";
        }
        logger_->Info(line + (car.hasWheels ? "" : " (no wheel meshes)"));
    }
    context.Set("gta5.vehicles.count",
                static_cast<int>(state_->vehicles.size()));
}

}  // namespace sdl3cpp::services::impl
