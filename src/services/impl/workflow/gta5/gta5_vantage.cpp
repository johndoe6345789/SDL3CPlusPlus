#include "services/interfaces/workflow/gta5/gta5_wanted_tiles.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace sdl3cpp::services::impl {
namespace {

/// Nine in ten of the tiles wanted now read and spawned. Not all: one
/// tile stuck on a mesh that never loads must not freeze the horizon.
bool MostlyIn(const Gta5StreamState& state) {
    std::size_t in = 0;
    for (const Gta5TileCoord& tile : state.wanted) {
        const auto found = state.resident.find(tile);
        if (found != state.resident.end() && found->second.placementsRead &&
            found->second.spawnedCount >= found->second.placements.size()) {
            ++in;
        }
    }
    return in * 10 >= state.wanted.size() * 9;
}

}  // namespace

int Gta5VantageTiles(const Gta5StreamState& state, float height) {
    const Gta5StreamingConfig& s = state.streaming;
    const int current = state.vantageTiles;
    if (s.vantageMetresPerTile <= 0.f) return 0;
    const float raw = (height - s.vantageBaseMetres) / s.vantageMetresPerTile;
    const int target = std::clamp(static_cast<int>(std::floor(raw)), 0,
                                  std::max(0, s.vantageMaxTiles));
    if (target < current) {
        return raw > static_cast<float>(current) - 0.5f ? current : target;
    }
    return target > current && MostlyIn(state) ? current + 1 : current;
}

}  // namespace sdl3cpp::services::impl
