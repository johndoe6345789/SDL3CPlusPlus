#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

/// Which edges of its cell a tile's road reaches, as a bit mask.
enum StuntsLink : std::uint8_t {
    kStuntsLinkNone = 0,
    kStuntsLinkNorth = 1 << 0,
    kStuntsLinkEast = 1 << 1,
    kStuntsLinkSouth = 1 << 2,
    kStuntsLinkWest = 1 << 3,
};

enum class StuntsTileKind : std::uint8_t {
    None,
    Straight,
    Corner,
    Junction,
};

struct StuntsTile {
    StuntsTileKind kind = StuntsTileKind::None;
    std::uint8_t links = kStuntsLinkNone;
};

/**
 * @brief What each of the 256 road tile ids connects to.
 *
 * The ids are opaque in the game's files, so the table ships as a JSON
 * asset derived from an install by packages/stunts/tools/
 * make_tile_table.py, which infers each id's directions from which
 * neighbours are occupied wherever that id appears across every track.
 */
struct StuntsTileTable {
    std::array<StuntsTile, 256> tiles{};
    int known = 0;
    bool loaded = false;
};

/// Reads the JSON table at `path`.
StuntsTileTable LoadStuntsTileTable(const std::string& path);

/// The opposite edge of `link`, for walking from tile to tile.
std::uint8_t StuntsOppositeLink(std::uint8_t link);

}  // namespace sdl3cpp::services::impl
