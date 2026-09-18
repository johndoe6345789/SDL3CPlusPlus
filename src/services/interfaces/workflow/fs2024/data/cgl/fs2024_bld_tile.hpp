#pragma once

#include <cstdint>
#include <vector>

namespace sdl3cpp::fs2024 {

/// A vertex in tile grid units: 1/16384 of the tile each, measured from
/// the tile's CENTRE, +x east and +y NORTH. Established against ground
/// truth, not assumed: with that mapping 89% of a Westminster tile's
/// building centres land inside real OpenStreetMap buildings (chance is
/// 13%), and its two largest footprints are County Hall and HM Treasury
/// where they really stand; a north-west-corner, y-south reading scores
/// at chance and mirrors every tile north-south. Buildings straddling an
/// edge are stored whole, so values run ~10% past +/-8192.
struct BldVertex {
    std::int32_t x = 0, y = 0;
};

/// One building as FS2024's own data has it.
struct BldBuilding {
    std::vector<std::uint32_t> ringSizes;  ///< [0] outer, rest holes
    std::vector<BldVertex> vertices;       ///< rings back to back
    std::uint16_t flags = 0;
    std::uint8_t type = 0;       ///< `building_type`, 'M'/'E'/numeric
    std::uint8_t roofType = 0;   ///< `roof_type`, 5 bits
    bool hasRoofType = false;    ///< false: this one was never surveyed
    std::uint8_t levels = 0;     ///< `building_levels`, 0 = not given
    bool hasRoofColour = false;
    std::uint8_t red = 0, green = 0, blue = 0;  ///< 5 bits each
};

struct BldTile {
    std::vector<BldBuilding> buildings;
    std::size_t bytesRead = 0;  ///< must equal the blob's size
};

/// Decodes one decompressed bld tile (see the CGL container reader
/// for where the blob comes from). `version` is the CGL header's own
/// (6 for everything FS2024 ships).
BldTile DecodeBldTile(const std::vector<std::uint8_t>& blob, int version);

}  // namespace sdl3cpp::fs2024
