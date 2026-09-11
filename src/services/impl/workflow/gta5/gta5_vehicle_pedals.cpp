#include "services/interfaces/workflow/gta5/gta5_vehicle.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_seat.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>

namespace sdl3cpp::services::impl {

Gta5Pedals DecideGta5Pedals(const Gta5Vehicle& car, float accelerate,
                            float slow, bool handbrake) {
    Gta5Pedals out;
    if (!car.chassis) return out;
    // How fast it goes the way it faces: +z is its forward.
    const btVector3 ahead =
        car.chassis->getWorldTransform().getBasis() * btVector3(0, 0, 1);
    const float forward = car.chassis->getLinearVelocity().dot(ahead);
    accelerate = std::clamp(accelerate, 0.f, 1.f);
    slow = std::clamp(slow, 0.f, 1.f);
    if (forward > 1.f) {
        // Rolling forward: the brake pedal brakes, as hard as pressed.
        out.throttle = slow > 0.f ? 0.f : accelerate;
        out.brake = slow;
    } else if (forward < -1.f) {
        // Rolling back: the throttle brakes first, the brake reverses.
        out.throttle = accelerate > 0.f ? 0.f : -slow;
        out.brake = accelerate;
    } else {
        // At a standstill either pedal drives: forward, or back.
        out.throttle = accelerate - slow;
    }
    // Backing up is slow going: gently, and no faster than 30 km/h.
    if (out.throttle < 0.f) out.throttle *= forward > -8.f ? 0.6f : 0.f;
    if (handbrake) {
        out.throttle = 0.f;
        out.brake = 1.f;
    }
    return out;
}

float ControlGta5Vehicle(Gta5Vehicle& car, WorkflowContext& context,
                         float dt) {
    const auto* keys = context.TryGet<nlohmann::json>("input.keyboard.state");
    const auto key = [&](const char* name) {
        return Gta5KeyDown(keys, name) ? 1.f : 0.f;
    };
    // Keys, or the pad's triggers and stick, progressively.
    const Gta5Pedals pedals = DecideGta5Pedals(
        car, std::max(key("W"), context.Get<float>("gta5.pad.throttle", 0.f)),
        std::max(key("S"), context.Get<float>("gta5.pad.brake", 0.f)),
        key("Space") > 0.f);
    // Bullet steers anticlockwise about up for a positive value, and the
    // car faces +z: a positive value turns it left, so A is positive.
    float steer = key("A") - key("D");
    if (steer == 0.f) steer = context.Get<float>("gta5.pad.steer", 0.f);
    DriveGta5Vehicle(car, pedals.throttle, steer, pedals.brake, dt);
    const btVector3 ahead =
        car.chassis->getWorldTransform().getBasis() * btVector3(0, 0, 1);
    const float speed = car.chassis->getLinearVelocity().dot(ahead);
    context.Set<float>("gta5.car.speed", speed);
    return speed;
}

}  // namespace sdl3cpp::services::impl
