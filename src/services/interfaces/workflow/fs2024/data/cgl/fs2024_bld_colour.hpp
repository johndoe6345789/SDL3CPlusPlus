#pragma once

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_cursor.hpp"
#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_tile.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::fs2024 {

/// The ring/hole tables, the 'E'-type side tables and the roof colour
/// palette sizes -- everything between the vertex counts and the
/// three colour columns. Fills each building's `ringSizes` and
/// returns, per building, how many colour entries it owns (a plain
/// building has one, an 'E' has a whole palette).
std::vector<std::uint32_t> ReadBldRingsAndPalettes(
    BldCursor& cursor, const std::vector<std::uint16_t>& flags,
    const std::vector<std::uint32_t>& vertexCounts,
    const std::vector<std::uint8_t>& types,
    std::vector<BldBuilding>& buildings);

/// The three parallel roof-colour columns (red, then green, then
/// blue; each a running byte column of `entries` values). Each
/// building takes the first entry of its own run -- for a palette
/// that is its dominant colour.
void ReadBldRoofColours(BldCursor& cursor,
                        const std::vector<std::uint16_t>& flags,
                        const std::vector<std::uint32_t>& colourEntries,
                        std::vector<BldBuilding>& buildings);

}  // namespace sdl3cpp::fs2024
