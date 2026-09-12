#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_path.hpp"

#include <array>
#include <iterator>

namespace sdl3cpp::services::impl {
namespace {

/// GTA's signals: 01a/b/d hang off a mast arm over the road, 03a/b sit
/// on a post at the kerb. Their placements put the base on the ground,
/// so the head is this far up each.
struct Gta5Signal {
    const char* archetype;
    float head;
};
constexpr Gta5Signal kSignals[] = {
    {"prop_traffic_01a", 5.4f}, {"prop_traffic_01b", 5.4f},
    {"prop_traffic_01d", 5.4f}, {"prop_traffic_02a", 5.4f},
    {"prop_traffic_03a", 3.1f}, {"prop_traffic_03b", 3.1f},
};

/// How far up a placement's signal head is, or nothing when it is not
/// a signal at all. A ymap carries only the archetype's hash, never its
/// name, so the names are hashed once here and matched on that; tiles
/// exported as JSON do carry the name, and are matched on either.
float HeadOf(const Gta5Placement& placement) {
    static const std::array<std::uint32_t, std::size(kSignals)> kHashes = [] {
        std::array<std::uint32_t, std::size(kSignals)> out{};
        for (std::size_t i = 0; i < std::size(kSignals); ++i) {
            out[i] = Gta5Hash(kSignals[i].archetype);
        }
        return out;
    }();
    for (std::size_t i = 0; i < std::size(kSignals); ++i) {
        if (placement.archetypeHash == kHashes[i] ||
            placement.archetype == kSignals[i].archetype) {
            return kSignals[i].head;
        }
    }
    return 0.f;
}

}  // namespace

void FindGta5Lamps(const Gta5StreamState& state, Gta5Traffic& traffic,
                   const Gta5Roads& roads, float dt) {
    Gta5Junction* looking = nullptr;
    for (Gta5Junction& junction : traffic.junctions) {
        junction.looked += dt;
        // Only the ones still without a signal, and only now and then:
        // the tile it stands in may not have streamed in yet.
        if (!junction.heads.empty() || junction.looked < 3.f) continue;
        if (!looking) looking = &junction;
    }
    if (!looking || looking->node >= roads.nodes.size()) return;
    looking->looked = 0.f;
    const glm::vec3 at = roads.nodes[looking->node].at;
    for (const auto& tile : state.resident) {
        for (const Gta5Placement& placement : tile.second.placements) {
            const float head = HeadOf(placement);
            if (head <= 0.f) continue;
            if (glm::distance(placement.position, at) > 26.f) continue;
            looking->heads.push_back(placement.position +
                                     glm::vec3(0.f, head, 0.f));
            if (looking->heads.size() >= 4u) return;
        }
    }
}

}  // namespace sdl3cpp::services::impl
