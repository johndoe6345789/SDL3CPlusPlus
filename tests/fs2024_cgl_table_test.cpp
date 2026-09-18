// ParseCglTable against hand-built tables: key deltas, the four
// compressed-size codes (small add, small subtract, escaped add,
// escaped subtract) and escaped uncompressed extras. Every one of
// these codes shows up in FS2024's own London building file.

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_cgl_table.hpp"

#include <gtest/gtest.h>

namespace tools = sdl3cpp::fs2024;

TEST(Fs2024CglTable, KeysAreRunningSumsOfDeltas) {
    const std::vector<std::uint16_t> words = {
        5, 1, 3,           // keys 5, 6, 9
        10, 0, 0,          // compressed 10, 10, 10
        4, 0, 2};          // uncompressed 14, 10, 12
    const auto tiles = tools::ParseCglTable(words, 3, 100);
    ASSERT_EQ(tiles.size(), 3u);
    EXPECT_EQ(tiles[0].key, 5u);
    EXPECT_EQ(tiles[1].key, 6u);
    EXPECT_EQ(tiles[2].key, 9u);
    EXPECT_EQ(tiles[0].uncompressedSize, 14u);
    EXPECT_EQ(tiles[1].uncompressedSize, 10u);
    EXPECT_EQ(tiles[2].uncompressedSize, 12u);
    EXPECT_EQ(tiles[0].offset, 100u);
    EXPECT_EQ(tiles[1].offset, 110u);
    EXPECT_EQ(tiles[2].offset, 120u);
}

TEST(Fs2024CglTable, SizeDeltasDecodeEveryEscape) {
    const std::vector<std::uint16_t> words = {
        0, 1, 1, 1,
        703,               // +703            -> 703
        0x8000 - 553,      // -553            -> 150
        0x8001, 4,         // +65536 + 4      -> 65690
        0xffff, 40,        // -65536 + 40     -> 194
        0x8002, 7,         // +131072 + 7
        0, 0, 0};
    const auto tiles = tools::ParseCglTable(words, 4, 0);
    EXPECT_EQ(tiles[0].compressedSize, 703u);
    EXPECT_EQ(tiles[1].compressedSize, 150u);
    EXPECT_EQ(tiles[2].compressedSize, 65690u);
    EXPECT_EQ(tiles[3].compressedSize, 194u);
    EXPECT_EQ(tiles[0].uncompressedSize, 703u + 131079u);
    EXPECT_EQ(tiles[3].uncompressedSize, 194u);
    EXPECT_EQ(tiles[3].offset, 703u + 150u + 65690u);
}

TEST(Fs2024CglTable, TruncatedTableThrows) {
    const std::vector<std::uint16_t> words = {1, 2};
    EXPECT_THROW(tools::ParseCglTable(words, 2, 0), std::runtime_error);
}
