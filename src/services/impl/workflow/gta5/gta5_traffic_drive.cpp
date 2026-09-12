#include "services/interfaces/workflow/gta5/gta5_traffic_path.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace sdl3cpp::services::impl {

void DriveGta5Traffic(Gta5Traffic& traffic, const Gta5Roads& roads,
                      float dt) {
    const glm::vec3 up(0.f, traffic.lift, 0.f);
    for (Gta5TrafficCar& car : traffic.cars) {
        if (car.from >= roads.nodes.size() ||
            car.to >= roads.nodes.size()) {
            continue;
        }
        const glm::vec3 a = roads.nodes[car.from].at;
        const glm::vec3 b = roads.nodes[car.to].at;
        const glm::vec3 run = b - a;
        const float length = glm::length(run);
        if (length < 0.5f) continue;
        const glm::vec3 way = run / length;
        const float heading = std::atan2(way.x, way.z);
        // Slow for the car in front, and hold at the line for a light
        // that is against this arm of the crossing.
        float want = car.want;
        const float gap = Gta5GapAhead(traffic, car);
        if (gap < 9.f) want = std::min(want, std::max(0.f, gap - 5.f) * 2.f);
        const float left = length - car.along;
        if (!Gta5LightOpen(traffic, car.to, heading) && left < 16.f) {
            want = std::min(want, std::max(0.f, left - 5.5f) * 1.6f);
        }
        // Eased, so they pull away and stop rather than snapping to it.
        const float ease = want > car.speed ? 2.6f : 7.0f;
        car.speed += (want - car.speed) * std::min(1.f, dt * ease);
        car.along += car.speed * dt;
        if (car.along >= length) {
            const std::uint32_t next = NextGta5Link(roads, car.to, car.from);
            car.along -= length;
            car.from = car.to;
            car.to = next;
        }
        const glm::vec3 side(way.z, 0.f, -way.x);  // its own right
        const glm::vec3 at = a + way * std::min(car.along, length) +
                             side * car.lane + up;
        const glm::mat4 model =
            glm::rotate(glm::translate(glm::mat4(1.f), at), heading,
                        glm::vec3(0.f, 1.f, 0.f));
        std::memcpy(car.instance.modelMatrix.data(), glm::value_ptr(model),
                    sizeof(float) * 16);
    }
}

}  // namespace sdl3cpp::services::impl
