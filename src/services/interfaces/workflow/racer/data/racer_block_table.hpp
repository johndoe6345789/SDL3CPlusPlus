#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// One entry of a Star Wars Episode I Racer lev01 block. `start` and
/// `end` are byte offsets into the block file; `end` is the next larger
/// offset in the table, or the end of the file.
struct RacerBlockEntry {
    std::uint32_t index = 0;
    std::size_t start = 0;
    std::size_t end = 0;
};

/// Reads the block table: a big-endian u32 count, then that many
/// big-endian u32 offsets. Returns an empty list if the table is
/// truncated or points outside the block.
std::vector<RacerBlockEntry> ReadRacerBlockTable(
    const std::vector<std::uint8_t>& block);

/// The four-character ASCII tag at the start of an entry, or an empty
/// string when the entry has none.
std::string RacerEntryTag(const std::vector<std::uint8_t>& block,
                          const RacerBlockEntry& entry);

}  // namespace sdl3cpp::services::impl
