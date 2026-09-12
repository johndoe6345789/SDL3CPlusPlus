#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_path.hpp"

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_seat.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// Where a car is now, or nowhere if it has no body yet.
glm::vec3 Standing(const Gta5TrafficCar& car) {
    const btVector3& at = car.car.chassis->getWorldTransform().getOrigin();
    return glm::vec3(at.x(), at.y(), at.z());
}

}  // namespace

void KeepGta5Traffic(Gta5Traffic& traffic, const Gta5Roads& roads,
                     Gta5StreamState& state, const glm::vec3& at,
                     SDL_GPUDevice* device, btDiscreteDynamicsWorld* world,
                     const std::shared_ptr<ILogger>& logger) {
    if (!roads.loaded || roads.nodes.empty() || !device || !world) return;
    // Let go of what has fallen behind, or what has been sitting on its
    // roof in a hedge for ten seconds: either way it is not traffic.
    for (std::size_t i = traffic.cars.size(); i-- > 0;) {
        Gta5TrafficCar& car = traffic.cars[i];
        const bool lost = !car.car.chassis ||
                          glm::distance(Standing(car), at) > traffic.far + 50.f;
        if (!lost && car.stuck < 10.f) continue;
        DestroyGta5Vehicle(car.car, world);
        traffic.cars.erase(traffic.cars.begin() +
                           static_cast<std::ptrdiff_t>(i));
    }
    if (static_cast<int>(traffic.cars.size()) >= traffic.want) return;
    std::uint32_t from = 0, to = 0;
    if (!Gta5TrafficSpot(roads, traffic, at, from, to)) return;
    // Facing the way the road runs, held over into the right-hand lane,
    // and set down a little high so it settles onto its suspension.
    const glm::vec3 a = roads.nodes[from].at, b = roads.nodes[to].at;
    const glm::vec3 run = b - a;
    const float length = glm::length(glm::vec2(run.x, run.z));
    if (length < 1.f) return;
    const glm::vec3 way = run / glm::length(run);
    Gta5TrafficCar car;
    car.from = from;
    car.to = to;
    car.want = 7.5f + static_cast<float>(from % 7u) * 0.6f;
    Gta5VehicleSpec spec;
    spec.model = traffic.model;
    spec.wheel = traffic.wheel;
    spec.paint = glm::vec3(0.55f, 0.57f, 0.60f);
    spec.wheelRadius = traffic.radius;
    spec.wheelWidth = traffic.width;
    spec.rideHeight = traffic.ride;
    spec.heading = glm::degrees(std::atan2(way.x, way.z));
    const glm::vec3 spot = a + glm::vec3(way.z, 0.f, -way.x) * car.lane +
                           glm::vec3(0.f, 1.2f, 0.f);
    if (!MakeGta5Vehicle(state, spec, spot, traffic.mass, device, world,
                         car.car, logger)) {
        return;
    }
    traffic.cars.push_back(car);
}

}  // namespace sdl3cpp::services::impl
