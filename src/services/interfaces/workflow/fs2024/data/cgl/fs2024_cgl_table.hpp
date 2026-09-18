#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sdl3cpp::fs2024 {

/// One object ("tile") of a CGL container: its quadkey index below
/// the container's own base quadkey, and where its stream sits.
struct CglTileEntry {
    std::uint32_t key = 0;  ///< base-4 digits below the base key
    std::uint64_t offset = 0;  ///< absolute file offset of its stream
    std::uint32_t compressedSize = 0;
    std::uint32_t uncompressedSize = 0;
};

/// Decodes a CGL's (already decompressed) data header: `count` u16
/// key deltas, then delta-coded compressed sizes, then uncompressed
/// sizes coded relative to each compressed size -- see the escape
/// rules in the .cpp. `dataStart` is where the first tile stream
/// begins (the tile streams follow each other with no gaps).
std::vector<CglTileEntry> ParseCglTable(
    const std::vector<std::uint16_t>& words, std::size_t count,
    std::uint64_t dataStart);

}  // namespace sdl3cpp::fs2024
