#include "services/interfaces/workflow/stunts/data/stunts_rle.hpp"

#include <gtest/gtest.h>

using sdl3cpp::services::impl::DecodeStuntsRle;

namespace {

/// Builds a minimal RLE header: id=1, `outSize`, a body length that is
/// computed from `body`'s size, and the ten escapes 0xF0..0xF9.
std::vector<std::uint8_t> Wrap(std::uint32_t outSize,
                               const std::vector<std::uint8_t>& body) {
    std::vector<std::uint8_t> raw = {
        1,
        static_cast<std::uint8_t>(outSize),
        static_cast<std::uint8_t>(outSize >> 8),
        static_cast<std::uint8_t>(outSize >> 16),
        0, 0, 0, 0,  // body length: unused by the decoder itself
        10,
        0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9,
    };
    raw.insert(raw.end(), body.begin(), body.end());
    return raw;
}

}  // namespace

TEST(StuntsRle, PassesThroughLiteralBytes) {
    const auto raw = Wrap(3, {'a', 'b', 'c'});
    const auto out = DecodeStuntsRle(raw.data(), raw.size());
    EXPECT_EQ(out, (std::vector<std::uint8_t>{'a', 'b', 'c'}));
}

TEST(StuntsRle, Escape0RepeatsByAnExplicitCount) {
    // esc0, count=5, value='x' -> five copies of 'x'.
    const auto raw = Wrap(5, {0xF0, 5, 'x'});
    const auto out = DecodeStuntsRle(raw.data(), raw.size());
    EXPECT_EQ(out, std::vector<std::uint8_t>(5, 'x'));
}

TEST(StuntsRle, Escape2RepeatsByA16BitCount) {
    // esc2, count=300 (0x012C little-endian), value='y'.
    const auto raw = Wrap(300, {0xF2, 0x2C, 0x01, 'y'});
    const auto out = DecodeStuntsRle(raw.data(), raw.size());
    EXPECT_EQ(out, std::vector<std::uint8_t>(300, 'y'));
}

TEST(StuntsRle, PositionEscapeRepeatsByItsOwnIndex) {
    // esc4 (index 4) -> exactly four copies of the value byte.
    const auto raw = Wrap(4, {0xF4, 'z'});
    const auto out = DecodeStuntsRle(raw.data(), raw.size());
    EXPECT_EQ(out, std::vector<std::uint8_t>(4, 'z'));
}

TEST(StuntsRle, SequenceRepeatsTheBytesBetweenTwoMarkers) {
    // esc1 opens, "ab", esc1 closes, count=3 -> "ababab".
    const auto raw = Wrap(6, {0xF1, 'a', 'b', 0xF1, 3});
    const auto out = DecodeStuntsRle(raw.data(), raw.size());
    EXPECT_EQ(out, (std::vector<std::uint8_t>{'a', 'b', 'a', 'b', 'a', 'b'}));
}

TEST(StuntsRle, NoSequenceFlagTreatsEscape1AsAnOrdinaryPositionEscape) {
    std::vector<std::uint8_t> raw = Wrap(1, {0xF1, 'q'});
    raw[8] |= 0x80;  // no_sequence flag
    const auto out = DecodeStuntsRle(raw.data(), raw.size());
    // Index 1 -> exactly one copy.
    EXPECT_EQ(out, std::vector<std::uint8_t>{'q'});
}

TEST(StuntsRle, SequenceContentCanContainOtherEscapes) {
    // A sequence's content is copied through unexpanded by the
    // sequence pass; the escape pass that follows then expands it.
    // Sequence content is esc4('c') = four 'c's; repeated twice.
    const auto raw = Wrap(8, {0xF1, 0xF4, 'c', 0xF1, 2});
    const auto out = DecodeStuntsRle(raw.data(), raw.size());
    EXPECT_EQ(out, std::vector<std::uint8_t>(8, 'c'));
}

TEST(StuntsRle, RejectsAWrongPassId) {
    std::vector<std::uint8_t> raw = Wrap(3, {'a', 'b', 'c'});
    raw[0] = 2;  // Huffman's id, not RLE's
    EXPECT_TRUE(DecodeStuntsRle(raw.data(), raw.size()).empty());
}

TEST(StuntsRle, RejectsATruncatedHeader) {
    const std::vector<std::uint8_t> raw = {1, 0, 0};
    EXPECT_TRUE(DecodeStuntsRle(raw.data(), raw.size()).empty());
}
