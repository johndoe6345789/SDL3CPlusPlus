#include "services/interfaces/workflow/switchback/race/switchback_race_restart_step.hpp"

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSwitchbackRaceRestartStep::WorkflowSwitchbackRaceRestartStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Gta5StreamState> vehicles,
    std::shared_ptr<SwitchbackSession> session)
    : logger_(std::move(logger)),
      vehicles_(std::move(vehicles)),
      session_(std::move(session)) {}

std::string WorkflowSwitchbackRaceRestartStep::GetPluginId() const {
    return "switchback.race.restart";
}

void WorkflowSwitchbackRaceRestartStep::Execute(
    const WorkflowStepDefinition&, WorkflowContext& context) {
    context.Set<bool>("switchback.race.restarted", false);
    if (!vehicles_ || vehicles_->vehicles.empty()) return;
    Gta5Vehicle& car = vehicles_->vehicles[0];
    if (!car.chassis) return;
    if (!haveStart_) {
        start_ = car.chassis->getWorldTransform();
        haveStart_ = true;
    }
    const auto* keys = context.TryGet<nlohmann::json>("input.keyboard.state");
    const bool down = Gta5KeyDown(keys, "R");
    const bool pressed = down && !keyWasDown_;
    keyWasDown_ = down;
    const bool requested = session_->restartRequested;
    session_->restartRequested = false;
    const bool keyed = pressed && session_->screen != SwitchbackScreen::Menu;
    if (!requested && !keyed) return;
    session_->screen = SwitchbackScreen::Race;
    Reset(car);
    context.Set<bool>("switchback.race.restarted", true);
}

void WorkflowSwitchbackRaceRestartStep::Reset(Gta5Vehicle& car) {
    car.chassis->setCenterOfMassTransform(start_);
    car.chassis->setLinearVelocity(btVector3(0.f, 0.f, 0.f));
    car.chassis->setAngularVelocity(btVector3(0.f, 0.f, 0.f));
    car.chassis->clearForces();
    car.chassis->activate(true);
    car.steer = 0.f;
    if (car.vehicle) {
        car.vehicle->resetSuspension();
        for (int i = 0; i < car.vehicle->getNumWheels(); ++i) {
            btWheelInfo& wheel = car.vehicle->getWheelInfo(i);
            wheel.m_rotation = 0.f;
            wheel.m_deltaRotation = 0.f;
            wheel.m_steering = 0.f;
            wheel.m_engineForce = 0.f;
            wheel.m_brake = 0.f;
        }
    }
    if (logger_) {
        logger_->Trace("WorkflowSwitchbackRaceRestartStep", "Reset",
                       "x=" + std::to_string(start_.getOrigin().x()) +
                           " z=" + std::to_string(start_.getOrigin().z()),
                       "Race restarted from the start line");
    }
}

}  // namespace sdl3cpp::services::impl
