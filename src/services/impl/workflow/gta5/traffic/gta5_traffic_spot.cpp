#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_path.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

bool Gta5TrafficSpot(const Gta5Roads& roads, const Gta5Traffic& traffic,
                     const glm::vec3& at, std::uint32_t& from,
                     std::uint32_t& to) {
    const int cx = Gta5RoadCellOf(at.x), cz = Gta5RoadCellOf(at.z);
    const int reach = static_cast<int>(traffic.far / 100.f) + 1;
    std::uint32_t seen = 0;
    for (int x = cx - reach; x <= cx + reach; ++x) {
        for (int z = cz - reach; z <= cz + reach; ++z) {
            const auto cell = roads.cells.find(Gta5RoadCell(x, z));
            if (cell == roads.cells.end()) continue;
            for (const std::uint32_t n : cell->second) {
                if (n >= roads.nodes.size()) continue;
                const Gta5RoadNode& node = roads.nodes[n];
                if (node.linkCount == 0u) continue;
                const float away = glm::distance(node.at, at);
                if (away < traffic.near || away > traffic.far) continue;
                const std::uint32_t next =
                    NextGta5Link(roads, n, roads.nodes.size());
                if (next == roads.nodes.size() || next == n) continue;
                // Never on top of a car already standing there, or the
                // two are dropped into each other and both are thrown.
                const bool taken = std::any_of(
                    traffic.cars.begin(), traffic.cars.end(),
                    [&](const Gta5TrafficCar& car) {
                        if (!car.car.chassis) return false;
                        const btVector3& o =
                            car.car.chassis->getWorldTransform().getOrigin();
                        return glm::distance(
                                   node.at,
                                   glm::vec3(o.x(), o.y(), o.z())) < 12.f;
                    });
                if (taken) continue;
                // Every candidate gets an even chance, rather than the
                // first one found always winning.
                if (Gta5TrafficRoll() % ++seen == 0u) {
                    from = n;
                    to = next;
                }
            }
        }
    }
    return seen > 0u;
}

}  // namespace sdl3cpp::services::impl
