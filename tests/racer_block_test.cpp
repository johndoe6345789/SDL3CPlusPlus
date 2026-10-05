#include "services/interfaces/workflow/racer/data/racer_block.hpp"

#include <gtest/gtest.h>

using sdl3cpp::services::impl::ReadRacerBlock;

namespace {

/// Two items of two parts. Item 0: [24,28) and [28,30). Item 1 has a
/// pixels part [30,32) and no palette (offset 0). Then the total size.
std::vector<std::uint8_t> TwoItemBlock() {
    std::vector<std::uint8_t> block = {
        0, 0, 0, 2,                    // item count
        0, 0, 0, 24, 0, 0, 0, 28,      // item 0 parts
        0, 0, 0, 30, 0, 0, 0, 0,       // item 1 parts (palette absent)
        0, 0, 0, 32};                  // total size
    block.resize(32, 0xAA);
    return block;
}

}  // namespace

TEST(RacerBlock, SplitsItemsIntoParts) {
    const auto items = ReadRacerBlock(TwoItemBlock(), 2);
    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0][0].start, 24u);
    EXPECT_EQ(items[0][0].end, 28u);
    EXPECT_EQ(items[0][1].start, 28u);
    EXPECT_EQ(items[0][1].end, 30u);
    EXPECT_EQ(items[1][0].start, 30u);
    EXPECT_EQ(items[1][0].end, 32u);
}

TEST(RacerBlock, MarksZeroOffsetsAbsent) {
    const auto items = ReadRacerBlock(TwoItemBlock(), 2);
    ASSERT_EQ(items.size(), 2u);
    EXPECT_TRUE(items[1][0].present);
    EXPECT_FALSE(items[1][1].present);
}

TEST(RacerBlock, RejectsTruncatedTable) {
    const std::vector<std::uint8_t> block = {0, 0, 0, 3, 0, 0, 0, 12};
    EXPECT_TRUE(ReadRacerBlock(block, 1).empty());
}

TEST(RacerBlock, RejectsOffsetPastEnd) {
    const std::vector<std::uint8_t> block = {0, 0, 0, 1, 0, 0, 0, 99,
                                             0, 0, 0, 12};
    EXPECT_TRUE(ReadRacerBlock(block, 1).empty());
}
