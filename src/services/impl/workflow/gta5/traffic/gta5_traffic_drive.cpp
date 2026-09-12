#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_path.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265f;

/// The point on the road a car is making for: its node, held over to
/// the right so the two directions pass rather than meet head on.
glm::vec3 Target(const Gta5Roads& roads, const Gta5TrafficCar& car) {
    const glm::vec3 to = roads.nodes[car.to].at;
    const glm::vec3 run = to - roads.nodes[car.from].at;
    const float length = glm::length(glm::vec2(run.x, run.z));
    if (length < 0.5f) return to;
    const glm::vec3 way = run / length;
    return to + glm::vec3(way.z, 0.f, -way.x) * car.lane;
}

}  // namespace

void DriveGta5Traffic(Gta5Traffic& traffic, const Gta5Roads& roads,
                      float dt) {
    for (Gta5TrafficCar& car : traffic.cars) {
        if (!car.car.chassis || car.to >= roads.nodes.size()) continue;
        const btTransform& shown = car.car.chassis->getWorldTransform();
        const btVector3& origin = shown.getOrigin();
        const glm::vec3 at(origin.x(), origin.y(), origin.z());
        const glm::vec3 aim = Target(roads, car);
        // The car's own forward: its mesh was turned to face +z, and
        // the chassis carries that round with it.
        const btVector3 nose = shown.getBasis() * btVector3(0.f, 0.f, 1.f);
        const float facing = std::atan2(nose.x(), nose.z());
        const float wanted = std::atan2(aim.x - at.x, aim.z - at.z);
        const float off = std::remainder(wanted - facing, 2.f * kPi);
        const btVector3& moving = car.car.chassis->getLinearVelocity();
        const float speed = std::sqrt(moving.x() * moving.x() +
                                      moving.z() * moving.z());
        // How fast it would like to be going, given the road ahead.
        float want = Gta5TrafficWant(traffic, roads, car, at, facing);
        // Slow into a bend rather than understeering through it.
        want *= std::max(0.35f, 1.f - std::fabs(off) * 0.7f);
        const float pedal = std::clamp((want - speed) * 0.5f, -1.f, 1.f);
        const float brake = (want < 0.5f && speed > 0.6f) ? 0.8f
                            : (pedal < -0.4f ? 0.35f : 0.f);
        DriveGta5Vehicle(car.car, std::max(0.f, pedal),
                         std::clamp(off * 1.4f, -1.f, 1.f), brake, dt);
        // Arrived, or close enough with the next road already opening
        // up: take the next one and carry on.
        if (glm::distance(glm::vec2(at.x, at.z), glm::vec2(aim.x, aim.z)) <
            7.f) {
            const std::uint32_t next =
                NextGta5Link(roads, car.to, car.from);
            car.from = car.to;
            car.to = next;
        }
        car.stuck = speed < 0.4f && want > 1.f ? car.stuck + dt : 0.f;
    }
}

}  // namespace sdl3cpp::services::impl
