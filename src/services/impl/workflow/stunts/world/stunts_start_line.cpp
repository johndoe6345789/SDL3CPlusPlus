#include "services/interfaces/workflow/stunts/world/stunts_start_line.hpp"

#include "services/interfaces/workflow/stunts/world/stunts_world_state.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// Heading that drives along a straight, in radians about +Y.
float HeadingFor(std::uint8_t links) {
    const bool northSouth =
        (links & kStuntsLinkNorth) && (links & kStuntsLinkSouth);
    return northSouth ? 1.57079632679f : 0.f;
}

}  // namespace

StuntsStartLine FindStuntsStartLine(const StuntsTrack& track,
                                    const StuntsTileTable& table,
                                    const StuntsMeshParams& params) {
    StuntsStartLine start;
    if (!track.loaded) return start;
    for (int y = 0; y < kStuntsGrid; ++y) {
        for (int x = 0; x < kStuntsGrid; ++x) {
            const StuntsTile& tile = table.tiles[StuntsRoadAt(track, x, y)];
            if (tile.kind != StuntsTileKind::Straight) continue;
            start.position = StuntsCellCentre(x, y, params.tileSize);
            start.position.y = params.roadHeight;
            start.heading = HeadingFor(tile.links);
            start.found = true;
            return start;
        }
    }
    return start;
}

}  // namespace sdl3cpp::services::impl
