#include "services/interfaces/workflow/switchback/vehicle/switchback_car_control_step.hpp"

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_seat.hpp"
#include "services/interfaces/workflow/switchback/vehicle/switchback_gearbox.hpp"
#include "services/interfaces/workflow/switchback/vehicle/switchback_wheel_damping.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <cmath>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

// gta5 pushes on the rear pair only, at 100 hp. Driving all four wheels at
// this multiple puts about 1,300 hp down, enough for 300 mph against drag.
constexpr float kDriveScale = 6.7f;
constexpr float kMphPerMetrePerSecond = 2.236936f;
constexpr int kTraceEveryFrames = 120;

// gta5 sets the rear engine force each step and leaves the front at zero.
// Copy the rear push to all four wheels, scaled.
void ApplyFourWheelDrive(Gta5Vehicle& car) {
    if (!car.vehicle) return;
    float rearForce = 0.f;
    for (int i = 0; i < car.vehicle->getNumWheels(); ++i) {
        const btWheelInfo& wheel = car.vehicle->getWheelInfo(i);
        if (!wheel.m_bIsFrontWheel) rearForce = wheel.m_engineForce;
    }
    for (int i = 0; i < car.vehicle->getNumWheels(); ++i) {
        car.vehicle->applyEngineForce(rearForce * kDriveScale, i);
    }
}

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
    const float dt = context.Get<float>("physics_dt", 1.f / 60.f);
    Gta5Vehicle& car = vehicles_->vehicles[0];
    const float speed = ControlGta5Vehicle(car, context, dt);
    ApplyFourWheelDrive(car);
    DampSwitchbackWheels(car);
    gearbox_.Update(std::abs(speed));
    context.Set<float>("gta5.car.revs", gearbox_.Revs());
    context.Set<int>("gta5.car.gear", gearbox_.Gear());
    context.Set("gta5.vehicle.seated", vehicles_->seated);
    TraceDrive(speed, gearbox_.Revs());
}

void WorkflowSwitchbackCarControlStep::TraceDrive(float speed, float revs) {
    if (!logger_ || ++framesSinceTrace_ < kTraceEveryFrames) return;
    framesSinceTrace_ = 0;
    logger_->Trace(
        "WorkflowSwitchbackCarControlStep", "TraceDrive",
        "mph=" + std::to_string(int(speed * kMphPerMetrePerSecond)) +
            " gear=" + std::to_string(gearbox_.Gear()),
        "revs=" + std::to_string(revs));
}

}  // namespace sdl3cpp::services::impl
