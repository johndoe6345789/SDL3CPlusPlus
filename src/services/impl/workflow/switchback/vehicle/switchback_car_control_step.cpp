#include "services/interfaces/workflow/switchback/vehicle/switchback_car_control_step.hpp"

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_seat.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSwitchbackCarControlStep::WorkflowSwitchbackCarControlStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Gta5StreamState> vehicles)
    : logger_(std::move(logger)), vehicles_(std::move(vehicles)) {}

std::string WorkflowSwitchbackCarControlStep::GetPluginId() const {
    return "switchback.car.control";
}

void WorkflowSwitchbackCarControlStep::Execute(
    const WorkflowStepDefinition&, WorkflowContext& context) {
    if (!vehicles_ || vehicles_->vehicles.empty()) return;
    if (vehicles_->seated < 0) vehicles_->seated = 0;
    const float dt = context.Get<float>("physics_dt", 1.f / 60.f);
    ControlGta5Vehicle(vehicles_->vehicles[0], context, dt);
    context.Set("gta5.vehicle.seated", vehicles_->seated);
}

}  // namespace sdl3cpp::services::impl
