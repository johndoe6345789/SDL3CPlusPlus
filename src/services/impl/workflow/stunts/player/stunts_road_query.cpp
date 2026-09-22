#include "services/interfaces/workflow/stunts/player/stunts_road_query.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// Grid cell containing `at`, or -1 when it is off the board.
int CellOf(float coordinate, float tileSize) {
    const float span = static_cast<float>(kStuntsGrid) * tileSize;
    const float shifted = coordinate + span * 0.5f;
    if (shifted < 0.f || shifted >= span) return -1;
    return static_cast<int>(shifted / tileSize);
}

}  // namespace

bool StuntsOnRoad(const StuntsWorldState& state, const glm::vec3& at) {
    const float tile = state.params.tileSize;
    const int x = CellOf(at.x, tile);
    const int y = CellOf(at.z, tile);
    if (x < 0 || y < 0) return false;
    const StuntsTile& cell = state.table.tiles[StuntsRoadAt(state.track, x, y)];
    return cell.kind != StuntsTileKind::None;
}

glm::vec3 StuntsClampToGrid(const StuntsWorldState& state,
                            const glm::vec3& at) {
    const float half =
        static_cast<float>(kStuntsGrid) * state.params.tileSize * 0.5f;
    return {std::clamp(at.x, -half, half), at.y,
            std::clamp(at.z, -half, half)};
}

}  // namespace sdl3cpp::services::impl
