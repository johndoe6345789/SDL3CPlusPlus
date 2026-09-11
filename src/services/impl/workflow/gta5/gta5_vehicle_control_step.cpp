#include "services/interfaces/workflow/gta5/gta5_vehicle_control_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_vehicle.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_seat.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

/// input.keyboard.poll records only the keys that are down, by name.
bool Down(const nlohmann::json* keys, const char* name) {
    return keys && keys->contains(name);
}

btRigidBody* PlayerBody(WorkflowContext& context) {
    const std::string name =
        context.GetString("physics_player_body", "player");
    return context.Get<btRigidBody*>("physics_body_" + name, nullptr);
}

}  // namespace

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
    btRigidBody* player = PlayerBody(context);

    // Edge-triggered: holding F must not toggle every frame.
    const bool toggle = Down(keys, "F");
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
            if (logger_ && found >= 0) {
                logger_->Info("gta5.vehicle.control: in");
            }
        }
    }

    if (state_->seated < 0 ||
        state_->seated >= static_cast<int>(state_->vehicles.size())) {
        return;
    }

    Gta5Vehicle& car = state_->vehicles[state_->seated];
    const float throttle = (Down(keys, "W") ? 1.f : 0.f) -
                           (Down(keys, "S") ? 1.f : 0.f);
    const float steer = (Down(keys, "D") ? 1.f : 0.f) -
                        (Down(keys, "A") ? 1.f : 0.f);
    DriveGta5Vehicle(car, throttle, steer, Down(keys, "Space") ? 1.f : 0.f);
    RideGta5Vehicle(car, player);
    context.Set("gta5.vehicle.seated", state_->seated);
}

}  // namespace sdl3cpp::services::impl
