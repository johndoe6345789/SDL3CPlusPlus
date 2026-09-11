#include "services/interfaces/workflow/gta5/gta5_vehicle.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr float kEngineForce = 4000.f;
constexpr float kBrakeForce = 220.f;
constexpr float kMaxSteer = 0.45f;

}  // namespace

void DriveGta5Vehicle(Gta5Vehicle& car, float throttle, float steer,
                      float brake) {
    if (!car.vehicle) return;
    for (int i = 0; i < car.vehicle->getNumWheels(); ++i) {
        const bool front = car.vehicle->getWheelInfo(i).m_bIsFrontWheel;
        // Rear-wheel drive, front-wheel steering.
        car.vehicle->applyEngineForce(front ? 0.f : throttle * kEngineForce, i);
        car.vehicle->setBrake(brake * kBrakeForce, i);
        if (front) car.vehicle->setSteeringValue(steer * kMaxSteer, i);
    }
}

}  // namespace sdl3cpp::services::impl
