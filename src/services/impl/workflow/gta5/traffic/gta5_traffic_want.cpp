#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_path.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// How far ahead of `at` the nearest other car is, measured along the
/// way this one is pointing, or a clear road. Only what is genuinely in
/// front and roughly in line counts: a car on the other carriageway is
/// not something to brake for.
float Ahead(const Gta5Traffic& traffic, const Gta5TrafficCar& self,
            const glm::vec3& at, float facing) {
    const glm::vec2 way(std::sin(facing), std::cos(facing));
    float gap = 1000.f;
    for (const Gta5TrafficCar& other : traffic.cars) {
        if (&other == &self || !other.car.chassis) continue;
        const btVector3& o = other.car.chassis->getWorldTransform().getOrigin();
        const glm::vec2 step(o.x() - at.x, o.z() - at.z);
        const float along = glm::dot(step, way);
        if (along <= 0.f) continue;
        const float across = std::fabs(step.x * way.y - step.y * way.x);
        if (across > 2.2f) continue;
        gap = std::min(gap, along);
    }
    return gap;
}

}  // namespace

float Gta5TrafficWant(const Gta5Traffic& traffic, const Gta5Roads& roads,
                      const Gta5TrafficCar& car, const glm::vec3& at,
                      float facing) {
    float want = car.want;
    // Off the throttle for the car in front, and stopped well before
    // running into it.
    const float gap = Ahead(traffic, car, at, facing);
    if (gap < 12.f) want = std::min(want, std::max(0.f, gap - 6.f) * 1.6f);
    // And held at the line when the light is against this arm. The
    // node it is making for is the crossing, so the distance to it is
    // the distance to the line.
    if (car.to < roads.nodes.size() &&
        !Gta5LightOpen(traffic, car.to, facing)) {
        const float line =
            glm::distance(glm::vec2(at.x, at.z),
                          glm::vec2(roads.nodes[car.to].at.x,
                                    roads.nodes[car.to].at.z));
        if (line < 22.f) {
            want = std::min(want, std::max(0.f, line - 7.f) * 1.1f);
        }
    }
    return want;
}

}  // namespace sdl3cpp::services::impl
