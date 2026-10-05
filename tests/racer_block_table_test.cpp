#include "services/interfaces/workflow/racer/data/racer_block_table.hpp"

#include <gtest/gtest.h>

using sdl3cpp::services::impl::ReadRacerBlockTable;
using sdl3cpp::services::impl::RacerEntryTag;

namespace {

/// Two entries: [12,16) and [16,24), the second tagged "Modl".
std::vector<std::uint8_t> TwoEntryBlock() {
    return {0, 0, 0, 2,            // count
            0, 0, 0, 12,           // entry 0 at 12
            0, 0, 0, 16,           // entry 1 at 16
            'x', 'x', 'x', 'x',    // entry 0 payload
            'M', 'o', 'd', 'l'};   // entry 1 payload
}

}  // namespace

TEST(RacerBlockTable, EntriesRunToTheNextOffset) {
    const auto block = TwoEntryBlock();
    const auto entries = ReadRacerBlockTable(block);
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0].start, 12u);
    EXPECT_EQ(entries[0].end, 16u);
    EXPECT_EQ(entries[1].start, 16u);
    EXPECT_EQ(entries[1].end, 20u);  // end of block
}

TEST(RacerBlockTable, ReadsFourCharacterTags) {
    const auto block = TwoEntryBlock();
    const auto entries = ReadRacerBlockTable(block);
    EXPECT_EQ(RacerEntryTag(block, entries[0]), "xxxx");
    EXPECT_EQ(RacerEntryTag(block, entries[1]), "Modl");
}

TEST(RacerBlockTable, RejectsTruncatedTable) {
    const std::vector<std::uint8_t> block = {0, 0, 0, 3, 0, 0, 0, 12};
    EXPECT_TRUE(ReadRacerBlockTable(block).empty());
}

TEST(RacerBlockTable, RejectsOffsetPastEnd) {
    const std::vector<std::uint8_t> block = {0, 0, 0, 1, 0, 0, 0, 99};
    EXPECT_TRUE(ReadRacerBlockTable(block).empty());
}
