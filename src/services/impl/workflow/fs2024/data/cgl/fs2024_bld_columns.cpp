#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_columns.hpp"

namespace sdl3cpp::fs2024 {
namespace {

constexpr int kFlagBits = 9;

}  // namespace

void SkipBldSubCells(BldCursor& cursor) {
    const std::uint8_t cells = cursor.U8();
    cursor.Skip(static_cast<std::size_t>(cells) * 2);  // two id bytes each
    std::int64_t running = 0, total = 0;
    for (std::uint8_t i = 0; i < cells; ++i) {
        running += cursor.DeltaVarint16();
        total += running;
    }
    for (std::int64_t i = 0; i < total; ++i) cursor.Varint8();
}

std::vector<std::uint16_t> ReadBldFlags(BldCursor& cursor,
                                        std::size_t buildings, int version) {
    std::vector<std::uint16_t> flags(buildings, 0);
    if (version < 6) {
        for (std::uint16_t& flag : flags) flag = cursor.U8();
        return flags;
    }
    const std::uint16_t present = cursor.U16();
    const std::size_t planeBytes = (buildings + 7) / 8;
    for (int bit = 0; bit < kFlagBits; ++bit) {
        if (((present >> bit) & 1) == 0) continue;
        for (std::size_t byte = 0; byte < planeBytes; ++byte) {
            const std::uint8_t packed = cursor.U8();
            for (int inByte = 0; inByte < 8; ++inByte) {
                const std::size_t at = byte * 8 + inByte;
                if (at < buildings && ((packed >> inByte) & 1)) {
                    flags[at] |= static_cast<std::uint16_t>(1u << bit);
                }
            }
        }
    }
    return flags;
}

BldColumn ReadBldByteColumn(BldCursor& cursor,
                            const std::vector<std::uint16_t>& flags,
                            std::uint16_t mask) {
    BldColumn column;
    column.value.assign(flags.size(), 0);
    column.present.assign(flags.size(), false);
    std::uint8_t running = 0;
    for (std::size_t i = 0; i < flags.size(); ++i) {
        if (!(flags[i] & mask)) continue;
        running = static_cast<std::uint8_t>(running + cursor.U8());
        column.value[i] = running;
        column.present[i] = true;
    }
    return column;
}

}  // namespace sdl3cpp::fs2024
