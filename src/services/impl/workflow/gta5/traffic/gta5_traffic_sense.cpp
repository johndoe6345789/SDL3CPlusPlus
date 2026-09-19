#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_ai.hpp"
#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_path.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// How far ahead of `at` a body is along `way`, or a clear road when it
/// is not genuinely in front and roughly in line: something on the
/// other carriageway is not a thing to brake for.
float Along(const btRigidBody* body, const glm::vec3& at,
            const glm::vec2& way) {
    if (!body) return 1000.f;
    const btVector3& o = body->getWorldTransform().getOrigin();
    const glm::vec2 step(o.x() - at.x, o.z() - at.z);
    const float along = glm::dot(step, way);
    if (along <= 0.f) return 1000.f;
    const float across = std::fabs(step.x * way.y - step.y * way.x);
    return across > 2.2f ? 1000.f : along;
}

}  // namespace

float Gta5TrafficGapAhead(const Gta5Traffic& traffic,
                          const Gta5StreamState& state,
                          const Gta5TrafficCar& self, const glm::vec3& at,
                          float facing) {
    const glm::vec2 way(std::sin(facing), std::cos(facing));
    float gap = 1000.f;
    for (const Gta5TrafficCar& other : traffic.cars) {
        if (&other == &self) continue;
        gap = std::min(gap, Along(other.car.chassis, at, way));
    }
    for (const Gta5Vehicle& mine : state.vehicles) {
        gap = std::min(gap, Along(mine.chassis, at, way));
    }
    return gap;
}

Gta5DriverSense Gta5SenseDriver(const Gta5Traffic& traffic,
                                const Gta5StreamState& state,
                                const Gta5Roads& roads,
                                const Gta5TrafficCar& car,
                                const glm::vec3& at, float facing) {
    Gta5DriverSense sense;
    sense.gap = Gta5TrafficGapAhead(traffic, state, car, at, facing);
    sense.want = car.want;
    sense.follow = car.follow;
    sense.nerve = car.nerve;
    // The node it is making for is the crossing, so the distance to it
    // is the distance to the line.
    if (car.to < roads.nodes.size() &&
        !Gta5LightOpen(traffic, car.to, facing)) {
        const glm::vec3& node = roads.nodes[car.to].at;
        sense.line = glm::distance(glm::vec2(at.x, at.z),
                                   glm::vec2(node.x, node.z));
    }
    return sense;
}

}  // namespace sdl3cpp::services::impl
