#include "services/interfaces/workflow/gta5/stream/gta5_tile_order.hpp"

#include "services/interfaces/workflow/gta5/stream/gta5_grid.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

std::vector<Gta5TileCoord> OrderGta5TilesByDistance(
    const Gta5StreamState& state) {
    std::vector<Gta5TileCoord> ordered(state.wanted.begin(),
                                       state.wanted.end());
    std::sort(ordered.begin(), ordered.end(),
              [&state](const Gta5TileCoord& a, const Gta5TileCoord& b) {
                  const float da = glm::distance(
                      Gta5TileCentre(state.world, a), state.centreOrigin);
                  const float db = glm::distance(
                      Gta5TileCentre(state.world, b), state.centreOrigin);
                  return da < db;
              });
    return ordered;
}

}  // namespace sdl3cpp::services::impl
