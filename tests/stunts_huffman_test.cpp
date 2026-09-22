#include "services/interfaces/workflow/stunts/data/stunts_huffman.hpp"

#include <gtest/gtest.h>

using sdl3cpp::services::impl::DecodeStuntsHuffman;
using sdl3cpp::services::impl::ReadStuntsHuffmanTable;
using sdl3cpp::services::impl::StuntsHuffmanTable;

namespace {

/// A complete two-symbol code: one count byte (lengths start at 1),
/// then the symbols in code order, then an MSB-first bit stream.
std::vector<std::uint8_t> TinyStream() {
    return {1,            // one code length in the table
            2,            // two codes of length 1
            'A', 'B',     // symbol for code 0, then code 1
            0x40};        // bits 0,1,0,0,0,0,0,0 -> A B A A A A A A
}

}  // namespace

TEST(StuntsHuffman, ReadsCountsForLengthsOneUpwards) {
    const auto raw = TinyStream();
    const auto table = ReadStuntsHuffmanTable(raw.data(), raw.size());
    ASSERT_TRUE(table.valid);
    // One width byte + one count byte + two symbols.
    EXPECT_EQ(table.headerBytes, 4u);
    EXPECT_EQ(table.symbols.size(), 2u);
}

TEST(StuntsHuffman, DecodesMsbFirst) {
    const auto raw = TinyStream();
    const auto table = ReadStuntsHuffmanTable(raw.data(), raw.size());
    ASSERT_TRUE(table.valid);
    const auto out =
        DecodeStuntsHuffman(table, raw.data(), raw.size(), 3);
    ASSERT_EQ(out.size(), 3u);
    EXPECT_EQ(out[0], 'A');
    EXPECT_EQ(out[1], 'B');
    EXPECT_EQ(out[2], 'A');
}

TEST(StuntsHuffman, RejectsAnOverSubscribedCode) {
    // Reading one count byte too many is what an off-by-one does: the
    // first symbol is taken as a count and the code no longer closes.
    std::vector<std::uint8_t> raw = {1, 2, 'A', 'B', 0x40};
    raw[0] = 2;   // claim two lengths, so 'A' is read as a count
    const auto table = ReadStuntsHuffmanTable(raw.data(), raw.size());
    EXPECT_FALSE(table.valid);
}

TEST(StuntsHuffman, RejectsTruncatedTables) {
    const std::vector<std::uint8_t> raw = {4, 0, 0};
    const auto table = ReadStuntsHuffmanTable(raw.data(), raw.size());
    EXPECT_FALSE(table.valid);
}
