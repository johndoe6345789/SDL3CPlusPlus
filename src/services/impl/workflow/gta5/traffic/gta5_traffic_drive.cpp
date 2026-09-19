#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_ai.hpp"
#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_path.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265f;

}  // namespace

void DriveGta5Traffic(Gta5Traffic& traffic, const Gta5Roads& roads,
                      const Gta5StreamState& state, float dt) {
    for (Gta5TrafficCar& car : traffic.cars) {
        if (!car.car.chassis) continue;
        if (car.to >= roads.nodes.size()) {
            // Nothing left to steer at. Counted as going nowhere so it
            // is recycled, rather than skipped every frame and left
            // standing in a lane for as long as the player is near.
            car.stuck += dt;
            continue;
        }
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
        const btVector3& turning = car.car.chassis->getAngularVelocity();
        Gta5DriverSense sense =
            Gta5SenseDriver(traffic, state, roads, car, at, facing);
        sense.speed = speed;
        sense.off = off;
        sense.turning = turning.y();
        const Gta5DriverAction act = Gta5DecideDrive(sense);
        DriveGta5Vehicle(car.car, act.throttle, act.steer, act.brake, dt);
        // Past the node it was making for: take the next road on.
        const glm::vec3 node = roads.nodes[car.to].at;
        if (glm::distance(glm::vec2(at.x, at.z),
                          glm::vec2(node.x, node.z)) < 8.f) {
            const std::uint32_t next = NextGta5Link(roads, car.to, car.from);
            car.from = car.to;
            car.to = next;
        }
        car.stuck =
            speed < 0.4f && act.throttle > 0.2f ? car.stuck + dt : 0.f;
    }
}

}  // namespace sdl3cpp::services::impl
