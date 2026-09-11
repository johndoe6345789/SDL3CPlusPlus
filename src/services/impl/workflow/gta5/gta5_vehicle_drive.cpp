#include "services/interfaces/workflow/gta5/gta5_vehicle.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

// A 100 hp saloon. An engine delivers power, not force: the push at the
// wheels is power over speed, so it is strongest pulling away and fades
// as the car gathers speed, until drag takes all of it.
constexpr float kPowerWatts = 74570.f;  // 100 hp
// Pulling away, grip rather than power is the limit.
constexpr float kMaxWheelForce = 6000.f;
// Air drag, 0.5 * rho * Cd * A: 0.5 * 1.2 kg/m3 * 0.30 * 2.2 m2.
constexpr float kDragPerSpeed2 = 0.40f;
// Rolling resistance, as a share of the car's weight.
constexpr float kRollingShare = 0.015f;
constexpr float kBrakeForce = 220.f;
constexpr float kMaxSteer = 0.45f;

}  // namespace

void DriveGta5Vehicle(Gta5Vehicle& car, float throttle, float steer,
                      float brake) {
    if (!car.vehicle || !car.chassis) return;
    const btVector3 velocity = car.chassis->getLinearVelocity();
    const float speed = velocity.length();
    const float inverse = car.chassis->getInvMass();
    const float weight = inverse > 0.f ? 9.81f / inverse : 0.f;

    // Shared by the two driven wheels. Held at 1 m/s from below, where
    // power over speed runs away to infinity.
    const float perWheel = std::min(
        kMaxWheelForce, kPowerWatts / (2.f * std::max(speed, 1.f)));
    for (int i = 0; i < car.vehicle->getNumWheels(); ++i) {
        const bool front = car.vehicle->getWheelInfo(i).m_bIsFrontWheel;
        // Rear-wheel drive, front-wheel steering.
        car.vehicle->applyEngineForce(front ? 0.f : throttle * perWheel, i);
        car.vehicle->setBrake(brake * kBrakeForce, i);
        if (front) car.vehicle->setSteeringValue(steer * kMaxSteer, i);
    }

    // Drag and rolling resistance oppose the motion. They set the top
    // speed, where drag meets the power: about 185 km/h for 100 hp.
    if (speed > 0.1f) {
        const float resist =
            kDragPerSpeed2 * speed * speed + kRollingShare * weight;
        car.chassis->applyCentralForce(-velocity / speed * resist);
    }
}

}  // namespace sdl3cpp::services::impl
