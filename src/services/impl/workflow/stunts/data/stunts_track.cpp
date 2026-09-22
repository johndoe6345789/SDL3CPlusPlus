#include "services/interfaces/workflow/stunts/data/stunts_track.hpp"

#include "services/interfaces/workflow/stunts/data/stunts_container.hpp"

namespace sdl3cpp::services::impl {

StuntsTrack LoadStuntsTrack(const std::string& path) {
    StuntsTrack track;
    const std::vector<std::uint8_t> raw = LoadStuntsFile(path);
    if (raw.size() != kStuntsTrackBytes) return track;

    for (int cell = 0; cell < kStuntsCells; ++cell) {
        track.road[static_cast<std::size_t>(cell)] = raw[cell];
        track.terrain[static_cast<std::size_t>(cell)] =
            raw[static_cast<std::size_t>(kStuntsCells + cell)];
    }
    track.horizon = raw[kStuntsCells * 2];
    track.flags = raw[kStuntsCells * 2 + 1];
    track.loaded = true;
    return track;
}

namespace {

bool InGrid(int x, int y) {
    return x >= 0 && x < kStuntsGrid && y >= 0 && y < kStuntsGrid;
}

}  // namespace

std::uint8_t StuntsRoadAt(const StuntsTrack& track, int x, int y) {
    if (!InGrid(x, y)) return 0;
    return track.road[static_cast<std::size_t>(y * kStuntsGrid + x)];
}

std::uint8_t StuntsTerrainAt(const StuntsTrack& track, int x, int y) {
    if (!InGrid(x, y)) return 0;
    return track.terrain[static_cast<std::size_t>(y * kStuntsGrid + x)];
}

}  // namespace sdl3cpp::services::impl
