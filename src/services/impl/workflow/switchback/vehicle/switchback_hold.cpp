#include "services/interfaces/workflow/switchback/vehicle/switchback_hold.hpp"

namespace sdl3cpp::services::impl {
namespace {

// Four times a full gta5 brake press, enough to hold a slope.
constexpr float kHoldBrake = 400.f;

}  // namespace

void HoldSwitchbackCar(Gta5Vehicle& car) {
    car.steer = 0.f;
    if (!car.vehicle) return;
    for (int i = 0; i < car.vehicle->getNumWheels(); ++i) {
        car.vehicle->applyEngineForce(0.f, i);
        car.vehicle->setSteeringValue(0.f, i);
        car.vehicle->setBrake(kHoldBrake, i);
    }
}

}  // namespace sdl3cpp::services::impl
