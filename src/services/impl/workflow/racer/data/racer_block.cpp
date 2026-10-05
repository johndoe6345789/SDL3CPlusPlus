#include "services/interfaces/workflow/racer/data/racer_block.hpp"

#include "services/interfaces/workflow/racer/data/racer_big_endian.hpp"

namespace sdl3cpp::services::impl {

std::vector<std::vector<RacerBlockPart>> ReadRacerBlock(
    const std::vector<std::uint8_t>& block, std::size_t partsPerItem) {
    const RacerBigEndianReader reader(block);
    if (partsPerItem == 0 || !reader.Has(0, 4)) return {};
    const std::size_t count = reader.U32(0);
    const std::size_t slots = count * partsPerItem;
    if (!reader.Has(4, 4 * slots + 4)) return {};

    std::vector<std::size_t> offsets(slots + 1);
    for (std::size_t i = 0; i < slots; ++i) {
        offsets[i] = reader.U32(4 + 4 * i);
        if (offsets[i] > block.size()) return {};
    }
    // The table's last word is the total size, which ends the last part.
    offsets[slots] = block.size();

    std::vector<std::vector<RacerBlockPart>> items(count);
    for (std::size_t slot = 0; slot < slots; ++slot) {
        RacerBlockPart part;
        if (offsets[slot] != 0) {
            std::size_t next = slot + 1;
            while (next < slots && offsets[next] == 0) ++next;
            part.start = offsets[slot];
            part.end = offsets[next];
            part.present = part.end >= part.start;
            if (!part.present) part = RacerBlockPart{};
        }
        items[slot / partsPerItem].push_back(part);
    }
    return items;
}

std::vector<std::uint8_t> RacerPartBytes(
    const std::vector<std::uint8_t>& block, const RacerBlockPart& part) {
    if (!part.present || part.end > block.size()) return {};
    return std::vector<std::uint8_t>(block.begin() + part.start,
                                     block.begin() + part.end);
}

}  // namespace sdl3cpp::services::impl
