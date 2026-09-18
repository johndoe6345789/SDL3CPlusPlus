#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_colour.hpp"

#include <numeric>

namespace sdl3cpp::fs2024 {
namespace {

constexpr std::uint16_t kFlagRings = 0x001;
constexpr std::uint16_t kFlagRoofColour = 0x020;
constexpr std::uint8_t kTypePalette = 0x45;  // 'E'

/// A running byte column of its own length, unconditioned by flags.
std::vector<std::uint8_t> RunningColumn(BldCursor& cursor,
                                        std::size_t count) {
    std::vector<std::uint8_t> values(count);
    std::uint8_t running = 0;
    for (std::uint8_t& value : values) {
        running = static_cast<std::uint8_t>(running + cursor.U8());
        value = running;
    }
    return values;
}

}  // namespace

std::vector<std::uint32_t> ReadBldRingsAndPalettes(
    BldCursor& cursor, const std::vector<std::uint16_t>& flags,
    const std::vector<std::uint32_t>& vertexCounts,
    const std::vector<std::uint8_t>& types,
    std::vector<BldBuilding>& buildings) {
    for (std::size_t i = 0; i < flags.size(); ++i) {
        std::uint32_t used = 0;
        if (flags[i] & kFlagRings) {
            const std::uint8_t holes = cursor.U8();
            for (std::uint8_t hole = 0; hole < holes; ++hole) {
                const std::uint8_t size = cursor.U8();
                buildings[i].ringSizes.push_back(size);
                used += size;
            }
        }
        buildings[i].ringSizes.insert(buildings[i].ringSizes.begin(),
                                     vertexCounts[i] - used);
    }
    for (std::size_t i = 0; i < flags.size(); ++i) {
        if (types[i] != kTypePalette || !(flags[i] & kFlagRings)) continue;
        cursor.Skip(cursor.U8());  // 'E' sub-group table
    }
    std::vector<std::uint32_t> entries(flags.size(), 0);
    std::size_t paletteTotal = 0;
    for (std::size_t i = 0; i < flags.size(); ++i) {
        if (!(flags[i] & kFlagRoofColour)) continue;
        entries[i] = types[i] == kTypePalette ? cursor.U8() : 1;
        if (types[i] == kTypePalette) paletteTotal += entries[i];
    }
    cursor.Skip(paletteTotal);  // palette weights
    return entries;
}

void ReadBldRoofColours(BldCursor& cursor,
                        const std::vector<std::uint16_t>& flags,
                        const std::vector<std::uint32_t>& colourEntries,
                        std::vector<BldBuilding>& buildings) {
    const std::size_t total = std::accumulate(colourEntries.begin(),
                                             colourEntries.end(),
                                             std::size_t{0});
    const auto red = RunningColumn(cursor, total);
    const auto green = RunningColumn(cursor, total);
    const auto blue = RunningColumn(cursor, total);
    std::size_t at = 0;
    for (std::size_t i = 0; i < flags.size(); ++i) {
        if (colourEntries[i] == 0) continue;
        buildings[i].hasRoofColour = true;
        buildings[i].red = red[at] & 0x1F;
        buildings[i].green = green[at] & 0x1F;
        buildings[i].blue = blue[at] & 0x1F;
        at += colourEntries[i];
    }
}

}  // namespace sdl3cpp::fs2024
