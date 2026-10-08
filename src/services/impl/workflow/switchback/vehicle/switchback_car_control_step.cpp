#include "services/interfaces/workflow/switchback/vehicle/switchback_car_control_step.hpp"

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_seat.hpp"
#include "services/interfaces/workflow/switchback/vehicle/switchback_gearbox.hpp"
#include "services/interfaces/workflow/switchback/vehicle/switchback_wheel_damping.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
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

// Slip is how much faster a wheel turns than the car moves, as a share of
// the car's speed. Traction control eases the drive from kSlipOnset, and
// keeps at least kMinTractionScale so the car can still pull away.
constexpr float kSlipOnset = 0.15f;
constexpr float kSlipRange = 0.35f;
constexpr float kMinTractionScale = 0.2f;
// Below this speed slip is measured against it, so a standing start is
// not read as endless wheelspin.
constexpr float kSlipSpeedFloor = 5.f;

// The throttle the driver is asking for, from the keys or the pad. gta5
// keeps its own pedal result private, so this repeats the input part of it.
float DriverThrottle(WorkflowContext& context) {
    const auto* keys = context.TryGet<nlohmann::json>("input.keyboard.state");
    if (Gta5KeyDown(keys, "S") ||
        context.Get<float>("gta5.pad.brake", 0.f) > 0.f) {
        return 0.f;
    }
    if (Gta5KeyDown(keys, "W")) return 1.f;
    return std::clamp(
        context.Get<float>("gta5.pad.throttle", 0.f), 0.f, 1.f);
}

float WheelSlip(const btWheelInfo& wheel, float forwardSpeed) {
    const float ground = std::abs(forwardSpeed);
    const float spin = std::abs(wheel.m_deltaRotation) * wheel.m_wheelsRadius;
    return (spin - ground) / std::max(ground, kSlipSpeedFloor);
}

float TractionScale(float slip) {
    const float over =
        std::clamp((slip - kSlipOnset) / kSlipRange, 0.f, 1.f);
    return std::max(kMinTractionScale, 1.f - over);
}

// gta5 sets the rear engine force each step and leaves the front at zero.
// Copy the rear push to all four wheels, scaled by the gearbox's rev limiter
// and by traction control, which eases off when any wheel spins. Returns the
// traction scale applied.
float ApplyFourWheelDrive(Gta5Vehicle& car, float powerScale,
                          float forwardSpeed) {
    if (!car.vehicle) return 1.f;
    float rearForce = 0.f;
    float tractionScale = 1.f;
    for (int i = 0; i < car.vehicle->getNumWheels(); ++i) {
        const btWheelInfo& wheel = car.vehicle->getWheelInfo(i);
        if (!wheel.m_bIsFrontWheel) rearForce = wheel.m_engineForce;
        tractionScale = std::min(
            tractionScale,
            TractionScale(WheelSlip(wheel, forwardSpeed)));
    }
    const float force = rearForce * kDriveScale * powerScale * tractionScale;
    for (int i = 0; i < car.vehicle->getNumWheels(); ++i) {
        car.vehicle->applyEngineForce(force, i);
    }
    return tractionScale;
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
