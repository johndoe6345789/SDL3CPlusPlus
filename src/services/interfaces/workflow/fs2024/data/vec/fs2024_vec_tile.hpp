#pragma once

#include <cstdint>
#include <vector>

namespace sdl3cpp::fs2024 {

/// One vector point, in the tile's own u16 frame -- see VecPointInTile
/// for where that lies.
struct VecPoint {
    std::uint16_t x = 0, y = 0;
};

/// One road, area or water outline.
struct VecFeature {
    std::uint32_t uid = 0;     ///< stable per feature (roads only)
    std::uint8_t flags = 0;    ///< 0x80 bridge, 0x40 tunnel (roads)
    std::uint8_t level = 0;    ///< a bridge's height above ground, 1..3
    int classBit = -1;         ///< importance: lower is grander
    std::vector<VecPoint> points;

    bool Bridge() const { return (flags & 0x80) != 0; }
};

/// The first three sections of a level-14 vector tile -- FS2024's own
/// OpenStreetMap-derived roads, land-use areas and water.
struct VecTile {
    std::vector<VecFeature> roads;  ///< polylines
    std::vector<VecFeature> areas;  ///< rings, not closed explicitly
    std::vector<VecFeature> water;  ///< rings: rivers, lakes, docks
};

/// Decodes a vector tile blob (see D:/fs2024/probe/vec_format.md):
/// sections of features, each a header (a class mask with cumulative
/// counts, or a plain count for water), fixed records ending in a
/// cumulative point count, then the points. The later sections (rail
/// and the generalised runs) are not read. A single zero byte is an
/// empty tile. Throws on a malformed blob.
VecTile DecodeVecTile(const std::vector<std::uint8_t>& blob);

}  // namespace sdl3cpp::fs2024
