#include "services/interfaces/workflow/switchback/vehicle/switchback_drive.hpp"

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_input.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

// gta5 pushes on the rear pair only, at 100 hp. Driving all four wheels at
// this multiple puts about 1,300 hp down, enough for 300 mph against drag.
constexpr float kDriveScale = 6.7f;

// Slip is how much faster a wheel turns than the car moves, as a share of
// the car's speed. Traction control eases the drive from kSlipOnset, and
// keeps at least kMinTractionScale so the car can still pull away.
constexpr float kSlipOnset = 0.15f;
constexpr float kSlipRange = 0.35f;
constexpr float kMinTractionScale = 0.2f;
// Below this speed slip is measured against it, so a standing start is
// not read as endless wheelspin.
constexpr float kSlipSpeedFloor = 5.f;

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

}  // namespace

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

}  // namespace sdl3cpp::services::impl
