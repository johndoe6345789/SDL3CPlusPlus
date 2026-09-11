#include "services/interfaces/workflow/gta5/gta5_vehicle_control_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_vehicle.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_seat.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

#include <utility>

namespace sdl3cpp::services::impl {
WorkflowGta5VehicleControlStep::WorkflowGta5VehicleControlStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5VehicleControlStep::GetPluginId() const {
    return "gta5.vehicle.control";
}

void WorkflowGta5VehicleControlStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    if (!state_ || state_->vehicles.empty()) return;

    const auto* keys = context.TryGet<nlohmann::json>("input.keyboard.state");
    btRigidBody* player = Gta5PlayerBody(context);

    // Edge-triggered: holding F must not toggle every frame.
    const bool toggle = Gta5KeyDown(keys, "F");
    const bool pressed = toggle && !toggleHeld_;
    toggleHeld_ = toggle;

    if (pressed && player) {
        if (state_->seated >= 0) {
            LeaveGta5Vehicle(state_->vehicles[state_->seated], player);
            state_->seated = -1;
            if (logger_) logger_->Info("gta5.vehicle.control: out");
        } else {
            const int found = FindGta5VehicleNear(
                *state_, player->getWorldTransform().getOrigin(), 6.f);
            state_->seated = found;
            // Said either way: a silent miss cannot be told apart from
            // a key that never arrived.
            if (logger_) {
                logger_->Info(found >= 0 ? "gta5.vehicle.control: in"
                                         : "gta5.vehicle.control: no car "
                                           "within 6 m");
            }
        }
    }

    // Every car nobody is in: engine off, brakes on. Nothing touched a
    // car once left, so it rolled off down any slope, or drove away on
    // the throttle held while getting out.
    for (std::size_t i = 0; i < state_->vehicles.size(); ++i) {
        if (static_cast<int>(i) == state_->seated) continue;
        DriveGta5Vehicle(state_->vehicles[i], 0.f, 0.f, 1.f);
    }
    if (state_->seated < 0 ||
        state_->seated >= static_cast<int>(state_->vehicles.size())) {
        return;
    }

    Gta5Vehicle& car = state_->vehicles[state_->seated];
    const float throttle = (Gta5KeyDown(keys, "W") ? 1.f : 0.f) -
                           (Gta5KeyDown(keys, "S") ? 1.f : 0.f);
    // Bullet steers anticlockwise about up for a positive value. The car
    // faces +z, so its right-hand side is -x and a positive value turns
    // it left: A is the positive key.
    const float steer = (Gta5KeyDown(keys, "A") ? 1.f : 0.f) -
                        (Gta5KeyDown(keys, "D") ? 1.f : 0.f);
    const float brake = Gta5KeyDown(keys, "Space") ? 1.f : 0.f;
    DriveGta5Vehicle(car, throttle, steer, brake);
    RideGta5Vehicle(car, player);
    context.Set("gta5.vehicle.seated", state_->seated);
}

}  // namespace sdl3cpp::services::impl
