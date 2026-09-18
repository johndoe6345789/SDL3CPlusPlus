#pragma once

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_cursor.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::fs2024 {

/// The sub-cell table at the head of a tile: a quadkey-Morton index
/// used for building uids and grouping. Nothing in this engine needs
/// a uid, but its bytes must still be stepped over exactly.
void SkipBldSubCells(BldCursor& cursor);

/// The per-building flag word, stored as up to nine separate
/// bitplanes, each one bit per building, LSB-first within a byte.
/// `maskPresent` (a u16) says which planes are actually there.
std::vector<std::uint16_t> ReadBldFlags(BldCursor& cursor,
                                        std::size_t buildings, int version);

/// One attribute column, and which buildings actually have it.
struct BldColumn {
    std::vector<std::uint8_t> value;  ///< running total when present
    std::vector<bool> present;
};

/// Reads one attribute column: a byte per building that carries
/// `mask`, each a delta on a running total mod 256. Buildings without
/// the flag consume nothing and have no value of their own -- a
/// terrace with no surveyed storey count must not inherit its
/// neighbour's, or a row of houses ends up as tower blocks.
BldColumn ReadBldByteColumn(BldCursor& cursor,
                            const std::vector<std::uint16_t>& flags,
                            std::uint16_t mask);

}  // namespace sdl3cpp::fs2024
