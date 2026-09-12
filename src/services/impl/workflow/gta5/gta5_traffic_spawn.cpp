#include "services/interfaces/workflow/gta5/gta5_traffic_path.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// A road within the ring where a car may appear: far enough out not
/// to pop into view, near enough to be worth having. Every candidate
/// gets an equal chance rather than the first found winning, or the
/// whole of the traffic queues onto one road and blocks itself.
bool Somewhere(const Gta5Roads& roads, const Gta5Traffic& traffic,
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
                // Not on top of one already waiting to pull away.
                const bool taken = std::any_of(
                    traffic.cars.begin(), traffic.cars.end(),
                    [&](const Gta5TrafficCar& car) {
                        return car.from == n && car.along < 14.f;
                    });
                if (taken) continue;
                if (Gta5TrafficRoll() % ++seen == 0u) {
                    from = n;
                    to = next;
                }
            }
        }
    }
    return seen > 0u;
}

}  // namespace

void KeepGta5Traffic(Gta5Traffic& traffic, const Gta5Roads& roads,
                     const glm::vec3& at) {
    if (!roads.loaded || roads.nodes.empty()) return;
    // Let go of anything that has fallen behind, wherever it had got to.
    const auto gone = std::remove_if(
        traffic.cars.begin(), traffic.cars.end(),
        [&](const Gta5TrafficCar& car) {
            return car.from >= roads.nodes.size() ||
                   glm::distance(roads.nodes[car.from].at, at) >
                       traffic.far + 40.f;
        });
    traffic.cars.erase(gone, traffic.cars.end());
    // One a frame at most, so a drive into a new district fills up over
    // a second or so rather than stalling the frame it arrives.
    if (static_cast<int>(traffic.cars.size()) >= traffic.want) return;
    std::uint32_t from = 0, to = 0;
    if (!Somewhere(roads, traffic, at, from, to)) return;
    Gta5TrafficCar car;
    car.from = from;
    car.to = to;
    car.instance.geometry = traffic.body;
    // A little spread in what they will do, so they are not a convoy.
    car.want = 7.5f + static_cast<float>(from % 7u) * 0.6f;
    car.speed = car.want * 0.6f;
    traffic.cars.push_back(car);
}

}  // namespace sdl3cpp::services::impl
