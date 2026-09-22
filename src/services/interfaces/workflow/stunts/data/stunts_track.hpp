#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

/// Stunts lays every track out on the same fixed grid.
constexpr int kStuntsGrid = 30;
constexpr int kStuntsCells = kStuntsGrid * kStuntsGrid;
constexpr std::size_t kStuntsTrackBytes = kStuntsCells * 2 + 2;

/**
 * @brief One .TRK file: two 30x30 grids and two trailing bytes.
 *
 * The file has no header. Bytes 0..899 are the road tile id of each
 * cell, row-major from the north edge; bytes 900..1799 are the terrain
 * id over the same cells; the last two bytes select the horizon and
 * scenery theme.
 */
struct StuntsTrack {
    std::array<std::uint8_t, kStuntsCells> road{};
    std::array<std::uint8_t, kStuntsCells> terrain{};
    std::uint8_t horizon = 0;
    std::uint8_t flags = 0;
    bool loaded = false;
};

/// Reads `path`; `loaded` is false unless it is exactly 1802 bytes.
StuntsTrack LoadStuntsTrack(const std::string& path);

/// Road id at (x, y), or 0 outside the grid.
std::uint8_t StuntsRoadAt(const StuntsTrack& track, int x, int y);

/// Terrain id at (x, y), or 0 outside the grid.
std::uint8_t StuntsTerrainAt(const StuntsTrack& track, int x, int y);

}  // namespace sdl3cpp::services::impl
