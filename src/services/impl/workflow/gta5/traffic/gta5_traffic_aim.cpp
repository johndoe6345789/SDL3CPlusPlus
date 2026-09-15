#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_path.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// A node held over into the right-hand lane of the road running into
/// it, so the two directions pass rather than meet.
glm::vec3 InLane(const Gta5Roads& roads, std::uint32_t from,
                 std::uint32_t to, float lane) {
    const glm::vec3 a = roads.nodes[from].at, b = roads.nodes[to].at;
    const glm::vec3 run = b - a;
    const float length = glm::length(glm::vec2(run.x, run.z));
    if (length < 0.5f) return b;
    const glm::vec3 way = run / glm::length(run);
    return b + glm::vec3(way.z, 0.f, -way.x) * lane;
}

}  // namespace

glm::vec3 Gta5TrafficAim(const Gta5Roads& roads, const Gta5TrafficCar& car,
                         const glm::vec3& at, float speed) {
    // Steering straight at the next node makes a car saw about: it
    // arrives, swings hard at the one after, overshoots and comes back.
    // A driver looks further ahead the faster they go, so this aims at
    // a point that far down the road -- the next node while it is still
    // away, the one after once it is close.
    // Both ends checked before either is read: a link off the end of
    // the network would otherwise be indexed on the way to the guard.
    if (car.from >= roads.nodes.size() || car.to >= roads.nodes.size()) {
        return at;
    }
    const glm::vec3 here = InLane(roads, car.from, car.to, car.lane);
    const float look = std::clamp(speed * 1.1f, 7.f, 20.f);
    const float left = glm::distance(glm::vec2(at.x, at.z),
                                     glm::vec2(here.x, here.z));
    if (left > look) return here;
    const std::uint32_t after = NextGta5Link(roads, car.to, car.from);
    if (after >= roads.nodes.size() || after == car.to) return here;
    const glm::vec3 next = InLane(roads, car.to, after, car.lane);
    // Part way onto the next road, by how far past the corner it is.
    const float into = std::clamp((look - left) / look, 0.f, 1.f);
    return here + (next - here) * into;
}

}  // namespace sdl3cpp::services::impl
