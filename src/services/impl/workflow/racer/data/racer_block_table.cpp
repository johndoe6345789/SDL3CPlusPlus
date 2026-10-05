#include "services/interfaces/workflow/racer/data/racer_block_table.hpp"

#include <set>

namespace sdl3cpp::services::impl {
namespace {

std::uint32_t ReadBigEndianU32(const std::vector<std::uint8_t>& data,
                               std::size_t offset) {
    return (static_cast<std::uint32_t>(data[offset]) << 24) |
           (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
           (static_cast<std::uint32_t>(data[offset + 2]) << 8) |
           static_cast<std::uint32_t>(data[offset + 3]);
}

}  // namespace

std::vector<RacerBlockEntry> ReadRacerBlockTable(
    const std::vector<std::uint8_t>& block) {
    if (block.size() < 4) return {};
    const std::size_t count = ReadBigEndianU32(block, 0);
    if (block.size() < 4 + 4 * count) return {};

    std::vector<std::size_t> offsets;
    std::set<std::size_t> boundaries{block.size()};
    offsets.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t offset = ReadBigEndianU32(block, 4 + 4 * i);
        if (offset >= block.size()) return {};
        offsets.push_back(offset);
        boundaries.insert(offset);
    }

    // Texture offsets are not in ascending order, so an entry ends at the
    // nearest boundary above its start, not at the next table slot.
    std::vector<RacerBlockEntry> entries;
    entries.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t start = offsets[i];
        const auto next = boundaries.upper_bound(start);
        entries.push_back({static_cast<std::uint32_t>(i), start,
                           next == boundaries.end() ? block.size() : *next});
    }
    return entries;
}

std::string RacerEntryTag(const std::vector<std::uint8_t>& block,
                          const RacerBlockEntry& entry) {
    if (entry.end - entry.start < 4) return {};
    std::string tag;
    for (std::size_t i = 0; i < 4; ++i) {
        const std::uint8_t c = block[entry.start + i];
        if (c < 32 || c >= 127) return {};
        tag.push_back(static_cast<char>(c));
    }
    return tag;
}

}  // namespace sdl3cpp::services::impl
