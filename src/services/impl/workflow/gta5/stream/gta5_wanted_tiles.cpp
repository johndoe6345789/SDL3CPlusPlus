#include "services/interfaces/workflow/gta5/stream/gta5_wanted_tiles.hpp"

#include "services/interfaces/workflow/gta5/stream/gta5_grid.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

using Gta5TileScores =
    std::unordered_map<Gta5TileCoord, float, Gta5TileCoordHash>;

float PlanarDistance(const glm::vec3& a, const glm::vec3& b) {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}

// A disc rather than the full square: the corners of a square sit 1.41x
// further out than the sides, so they cost tiles that are always the
// first to be evicted again.
void AddDisc(const Gta5StreamState& state, const glm::vec3& centre,
             int radius, float loadDistance, Gta5TileScores& scores) {
    const Gta5TileCoord at = Gta5TileForPosition(state.world, centre);
    for (int dz = -radius; dz <= radius; ++dz) {
        for (int dx = -radius; dx <= radius; ++dx) {
            const Gta5TileCoord tile{at.x + dx, at.z + dz};
            const float distance =
                PlanarDistance(Gta5TileCentre(state.world, tile), centre);
            if (distance > loadDistance) continue;
            const auto [it, fresh] = scores.emplace(tile, distance);
            if (!fresh) it->second = std::min(it->second, distance);
        }
    }
}

}  // namespace

void ResolveGta5WantedTiles(Gta5StreamState& state, const glm::vec3& origin,
                            const glm::vec3& lead) {
    state.vantageTiles = Gta5VantageTiles(state, origin.y);
    const int radius =
        std::max(0, state.streaming.loadRadiusTiles) + state.vantageTiles;
    const float tileSize =
        state.world.tileSize > 0.f ? state.world.tileSize : 512.f;
    const float loadDistance =
        (static_cast<float>(radius) + 0.5f) * tileSize;
    Gta5TileScores scores;
    AddDisc(state, origin, radius, loadDistance, scores);
    AddDisc(state, lead, radius, loadDistance, scores);

    std::vector<std::pair<Gta5TileCoord, float>> candidates(scores.begin(),
                                                             scores.end());
    // Under budget pressure the nearest tiles win, so the hole that opens
    // up is always the furthest away.
    const auto budget = static_cast<std::size_t>(
        std::max(1, state.streaming.maxResidentTiles));
    if (candidates.size() > budget) {
        std::partial_sort(
            candidates.begin(),
            candidates.begin() + static_cast<std::ptrdiff_t>(budget),
            candidates.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        candidates.resize(budget);
    }

    state.wanted.clear();
    state.wanted.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        state.wanted.insert(candidate.first);
    }
}

}  // namespace sdl3cpp::services::impl
