#include "services/interfaces/workflow/switchback/vehicle/switchback_car_control_step.hpp"

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_seat.hpp"
#include "services/interfaces/workflow/switchback/vehicle/switchback_drive.hpp"
#include "services/interfaces/workflow/switchback/vehicle/switchback_gearbox.hpp"
#include "services/interfaces/workflow/switchback/vehicle/switchback_wheel_damping.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <cmath>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kMphPerMetrePerSecond = 2.236936f;
constexpr int kTraceEveryFrames = 120;

}  // namespace

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
    if (context.GetBool("switchback.race.restarted", false)) {
        gearbox_ = SwitchbackGearbox();
    }
    const float dt = context.Get<float>("physics_dt", 1.f / 60.f);
    Gta5Vehicle& car = vehicles_->vehicles[0];
    const float throttle = DriverThrottle(context);
    const float speed = ControlGta5Vehicle(car, context, dt);
    const float traction =
        ApplyFourWheelDrive(car, gearbox_.PowerScale(), speed);
    DampSwitchbackWheels(car);
    gearbox_.Update(std::abs(speed), throttle, dt);
    context.Set<float>("gta5.car.revs", gearbox_.Revs());
    context.Set<float>("engine.revs", gearbox_.Revs());
    context.Set<int>("gta5.car.gear", gearbox_.Gear());
    context.Set("gta5.vehicle.seated", vehicles_->seated);
    TraceDrive(car, speed, traction);
}

void WorkflowSwitchbackCarControlStep::TraceDrive(const Gta5Vehicle& car,
                                                  float speed,
                                                  float traction) {
    if (!logger_ || ++framesSinceTrace_ < kTraceEveryFrames) return;
    framesSinceTrace_ = 0;
    logger_->Trace(
        "WorkflowSwitchbackCarControlStep", "TraceDrive",
        "mph=" + std::to_string(int(speed * kMphPerMetrePerSecond)) +
            " gear=" + std::to_string(gearbox_.Gear()),
        "revs=" + std::to_string(gearbox_.Revs()) +
            " traction=" + std::to_string(traction) + " " +
            DescribeSwitchbackSuspension(car));
}

}  // namespace sdl3cpp::services::impl
