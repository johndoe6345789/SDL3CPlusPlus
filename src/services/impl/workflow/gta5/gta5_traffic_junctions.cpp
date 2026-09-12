#include "services/interfaces/workflow/gta5/gta5_traffic.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265f;

float Folded(float radians) {
    float out = std::fmod(radians, kPi);
    return out < 0.f ? out + kPi : out;
}

}  // namespace

void Gta5FindJunctions(Gta5Traffic& traffic, const Gta5Roads& roads,
                       const glm::vec3& at) {
    // Anything with three ways out of it is a crossing, and takes the
    // bearing of its first road as the arm that goes first. Staggering
    // the clock by the node keeps a whole street from turning as one.
    // Only the cells around the player are looked at: the graph is the
    // whole of Los Santos and walking it every frame is not on.
    const int cx = Gta5RoadCellOf(at.x), cz = Gta5RoadCellOf(at.z);
    const int reach = static_cast<int>(traffic.far / 100.f) + 1;
    for (int x = cx - reach; x <= cx + reach; ++x) {
        for (int z = cz - reach; z <= cz + reach; ++z) {
            const auto cell = roads.cells.find(Gta5RoadCell(x, z));
            if (cell == roads.cells.end()) continue;
            for (const std::uint32_t n : cell->second) {
                if (n >= roads.nodes.size()) continue;
                const Gta5RoadNode& node = roads.nodes[n];
                if (node.linkCount < 3u) continue;
                if (glm::distance(node.at, at) > traffic.far) continue;
                // Three ways out is not enough on its own: GTA gives a
                // node to every driveway and lane split, and lighting
                // all of them stops a car every few metres. A crossing
                // worth a signal has real road on at least two of its
                // arms -- four lanes counting both ways.
                int wide = 0;
                for (std::uint8_t i = 0; i < node.linkCount; ++i) {
                    const Gta5RoadLink& arm = roads.links[node.firstLink + i];
                    if (arm.forward + arm.back >= 4) ++wide;
                }
                if (wide < 2) continue;
                const bool known = std::any_of(
                    traffic.junctions.begin(), traffic.junctions.end(),
                    [n](const Gta5Junction& j) { return j.node == n; });
                if (known) continue;
                const Gta5RoadLink& first = roads.links[node.firstLink];
                if (first.to >= roads.nodes.size()) continue;
                const glm::vec3 away = roads.nodes[first.to].at - node.at;
                Gta5Junction made;
                made.node = n;
                made.axis = Folded(std::atan2(away.x, away.z));
                made.clock = std::fmod(static_cast<float>(n % 97u) * 0.37f,
                                       traffic.cycle);
                traffic.junctions.push_back(made);
            }
        }
    }
}

}  // namespace sdl3cpp::services::impl
