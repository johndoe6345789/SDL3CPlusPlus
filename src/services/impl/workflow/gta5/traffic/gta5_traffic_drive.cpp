#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_path.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265f;

}  // namespace

void DriveGta5Traffic(Gta5Traffic& traffic, const Gta5Roads& roads,
                      float dt) {
    for (Gta5TrafficCar& car : traffic.cars) {
        if (!car.car.chassis || car.to >= roads.nodes.size()) continue;
        const btTransform& shown = car.car.chassis->getWorldTransform();
        const btVector3& origin = shown.getOrigin();
        const glm::vec3 at(origin.x(), origin.y(), origin.z());
        const btVector3& moving = car.car.chassis->getLinearVelocity();
        const float speed = std::sqrt(moving.x() * moving.x() +
                                      moving.z() * moving.z());
        // Its own forward: the mesh was turned to face +z, and the
        // chassis carries that round with it.
        const btVector3 nose = shown.getBasis() * btVector3(0.f, 0.f, 1.f);
        const float facing = std::atan2(nose.x(), nose.z());
        const glm::vec3 aim = Gta5TrafficAim(roads, car, at, speed);
        const float wanted = std::atan2(aim.x - at.x, aim.z - at.z);
        const float off = std::remainder(wanted - facing, 2.f * kPi);
        float want = Gta5TrafficWant(traffic, roads, car, at, facing);
        // Slow into a bend rather than understeering through it.
        want *= std::max(0.4f, 1.f - std::fabs(off) * 0.6f);
        // A softer hand the faster it goes, and damped by how fast it
        // is already turning, or it weaves down a straight road.
        const btVector3& turning = car.car.chassis->getAngularVelocity();
        const float gain = 1.6f / (1.f + speed * 0.12f);
        const float steer =
            std::clamp(off * gain - turning.y() * 0.25f, -1.f, 1.f);
        const float pedal = std::clamp((want - speed) * 0.5f, -1.f, 1.f);
        const float brake = (want < 0.5f && speed > 0.6f) ? 0.8f
                            : (pedal < -0.4f ? 0.35f : 0.f);
        DriveGta5Vehicle(car.car, std::max(0.f, pedal), steer, brake, dt);
        // Past the node it was making for: take the next road on.
        const glm::vec3 node = roads.nodes[car.to].at;
        if (glm::distance(glm::vec2(at.x, at.z),
                          glm::vec2(node.x, node.z)) < 8.f) {
            const std::uint32_t next = NextGta5Link(roads, car.to, car.from);
            car.from = car.to;
            car.to = next;
        }
        car.stuck = speed < 0.4f && want > 1.f ? car.stuck + dt : 0.f;
    }
}

}  // namespace sdl3cpp::services::impl
