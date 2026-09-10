#include "services/interfaces/workflow/gta5/gta5_wanted_tiles.hpp"

#include "services/interfaces/workflow/gta5/gta5_grid.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

struct ScoredTile {
    Gta5TileCoord tile;
    float distance{0.f};
};

float PlanarDistance(const glm::vec3& a, const glm::vec3& b) {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}

}  // namespace

void ResolveGta5WantedTiles(Gta5StreamState& state,
                            const glm::vec3& centre) {
    const int radius = std::max(0, state.streaming.loadRadiusTiles);
    const float tileSize =
        state.world.tileSize > 0.f ? state.world.tileSize : 512.f;
    // Take a disc rather than the full square: the corners of a square sit
    // 1.41x further out than the sides, so they cost tiles that are always
    // the first to be evicted again.
    const float loadDistance =
        (static_cast<float>(radius) + 0.5f) * tileSize;

    std::vector<ScoredTile> candidates;
    for (int dz = -radius; dz <= radius; ++dz) {
        for (int dx = -radius; dx <= radius; ++dx) {
            const Gta5TileCoord tile{state.centre.x + dx,
                                     state.centre.z + dz};
            const float distance =
                PlanarDistance(Gta5TileCentre(state.world, tile), centre);
            if (distance <= loadDistance) {
                candidates.push_back(ScoredTile{tile, distance});
            }
        }
    }

    // Under budget pressure the nearest tiles win, so the hole that opens
    // up is always the furthest away.
    const auto budget = static_cast<std::size_t>(
        std::max(1, state.streaming.maxResidentTiles));
    if (candidates.size() > budget) {
        std::partial_sort(
            candidates.begin(),
            candidates.begin() + static_cast<std::ptrdiff_t>(budget),
            candidates.end(),
            [](const ScoredTile& a, const ScoredTile& b) {
                return a.distance < b.distance;
            });
        candidates.resize(budget);
    }

    state.wanted.clear();
    state.wanted.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        state.wanted.insert(candidate.tile);
    }
}

}  // namespace sdl3cpp::services::impl
