#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_path.hpp"

#include <algorithm>
#include <random>

namespace sdl3cpp::services::impl {
namespace {

std::mt19937& Rng() {
    static std::mt19937 rng(20260912);
    return rng;
}

}  // namespace

std::uint32_t Gta5TrafficRoll() { return Rng()(); }

std::uint32_t NextGta5Link(const Gta5Roads& roads, std::uint32_t node,
                           std::uint32_t from) {
    if (node >= roads.nodes.size()) return from;
    const Gta5RoadNode& here = roads.nodes[node];
    std::uint32_t picks[8];
    std::uint8_t count = 0;
    for (std::uint8_t i = 0; i < here.linkCount && count < 8; ++i) {
        const Gta5RoadLink& link = roads.links[here.firstLink + i];
        if (link.to >= roads.nodes.size() || link.to == from) continue;
        picks[count++] = link.to;
    }
    if (count == 0) return from;
    return picks[Rng()() % count];
}

glm::vec3 Gta5TrafficPaint(std::uint32_t roll) {
    // The colours a street of parked cars actually is: mostly greys,
    // with the odd one that someone chose.
    static const glm::vec3 kPaints[] = {
        {0.82f, 0.83f, 0.85f}, {0.16f, 0.17f, 0.19f},
        {0.45f, 0.47f, 0.50f}, {0.62f, 0.64f, 0.66f},
        {0.30f, 0.33f, 0.38f}, {0.55f, 0.12f, 0.12f},
        {0.12f, 0.25f, 0.45f}, {0.20f, 0.35f, 0.25f},
    };
    return kPaints[roll % (sizeof(kPaints) / sizeof(kPaints[0]))];
}

}  // namespace sdl3cpp::services::impl
