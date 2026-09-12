#include "services/interfaces/workflow/gta5/gta5_traffic_path.hpp"

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

float Gta5GapAhead(const Gta5Traffic& traffic, const Gta5TrafficCar& car) {
    float gap = 1000.f;
    for (const Gta5TrafficCar& other : traffic.cars) {
        if (&other == &car || other.from != car.from ||
            other.to != car.to || other.along <= car.along) {
            continue;
        }
        gap = std::min(gap, other.along - car.along);
    }
    return gap;
}

}  // namespace sdl3cpp::services::impl
