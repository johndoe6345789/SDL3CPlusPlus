#include "services/interfaces/workflow/gta5/gta5_vehicle_control_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_finite_guard.hpp"
#include "services/interfaces/workflow/gta5/gta5_player_pin.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_seat.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
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
            // The movement state too, or it put them back where they got in.
            const btVector3& at = player->getWorldTransform().getOrigin();
            PinGta5Player(context, player, glm::vec3(at.x(), at.y(), at.z()));
            state_->seated = -1;
            if (logger_) logger_->Info("gta5.vehicle.control: out");
        } else {
            const int found = FindGta5VehicleNear(
                *state_, player->getWorldTransform().getOrigin(), 6.f);
            state_->seated = found;
            // Said either way: a miss must not look like a lost key.
            if (logger_) {
                logger_->Info(found >= 0 ? "gta5.vehicle.control: in"
                                         : "gta5.vehicle.control: no car "
                                           "within 6 m");
            }
        }
    }

    // Every car nobody is in: engine off, brakes on, or it rolls away.
    const float dt = context.Get<float>("physics_dt", 1.f / 60.f);
    for (std::size_t i = 0; i < state_->vehicles.size(); ++i) {
        if (static_cast<int>(i) == state_->seated) continue;
        DriveGta5Vehicle(state_->vehicles[i], 0.f, 0.f, 1.f, dt);
    }
    GuardGta5PlayerFinite(*state_, context, player, logger_);
    if (state_->seated < 0 ||
        state_->seated >= static_cast<int>(state_->vehicles.size())) {
        return;
    }

    Gta5Vehicle& car = state_->vehicles[state_->seated];
    const float speed = ControlGta5Vehicle(car, context, dt);
    if (logger_ && (reportIn_ -= dt) <= 0.f) {
        reportIn_ = 1.f;
        logger_->Info("gta5.vehicle.control: " +
                      std::to_string(int(speed * 3.6f)) + " km/h");
    }
    RideGta5Vehicle(car, player, context);
    context.Set("gta5.vehicle.seated", state_->seated);
}

}  // namespace sdl3cpp::services::impl
