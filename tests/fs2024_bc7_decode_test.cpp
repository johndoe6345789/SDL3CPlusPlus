// DecodeBc7Block against blocks packed by hand, bit by bit, following
// the BPTC spec -- endpoint p-bits, index weights and mode 5's
// channel rotation are where a BC7 decoder usually goes wrong.

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_bc7_decode.hpp"

#include <gtest/gtest.h>

#include <array>

namespace tools = sdl3cpp::fs2024;

namespace {

struct BitWriter {
    std::array<std::uint8_t, 16> block{};
    int at = 0;
    void Put(int value, int count) {
        for (int i = 0; i < count; ++i, ++at) {
            block[at >> 3] |= ((value >> i) & 1) << (at & 7);
        }
    }
};

std::array<std::uint8_t, 64> Decode(const BitWriter& w) {
    std::array<std::uint8_t, 64> out{};
    tools::DecodeBc7Block(w.block.data(), out.data(), 16);
    return out;
}

}  // namespace

TEST(Fs2024Bc7, Mode6EndpointsCarryTheirPBit) {
    BitWriter w;
    w.Put(1 << 6, 7);                    // mode 6
    for (int v : {100, 20, 50, 60, 0, 127, 127, 127}) w.Put(v, 7);
    w.Put(1, 1); w.Put(0, 1);            // p-bits
    w.Put(0, 3);                         // texel 0 (anchor), weight 0
    w.Put(15, 4);                        // texel 1, weight 64
    const auto px = Decode(w);
    EXPECT_EQ(px[0], 201);               // (100 << 1) | 1
    EXPECT_EQ(px[1], 101);               // (50 << 1) | 1
    EXPECT_EQ(px[2], 1);
    EXPECT_EQ(px[3], 255);
    EXPECT_EQ(px[4], 40);                // endpoint 1: (20 << 1) | 0
    EXPECT_EQ(px[5], 120);
    EXPECT_EQ(px[6], 254);
    EXPECT_EQ(px[7], 254);
}

TEST(Fs2024Bc7, Mode5RotationSwapsAlphaIntoRed) {
    BitWriter w;
    w.Put(1 << 5, 6);                    // mode 5
    w.Put(1, 2);                         // rotation 1: swap A and R
    for (int v : {127, 127, 0, 0, 64, 64}) w.Put(v, 7);
    w.Put(10, 8); w.Put(10, 8);          // alpha endpoints
    const auto px = Decode(w);           // all indices zero
    EXPECT_EQ(px[0], 10);                // red now holds alpha
    EXPECT_EQ(px[3], 255);               // alpha holds red (127 -> 255)
    EXPECT_EQ(px[1], 0);
    EXPECT_EQ(px[2], 129);               // 64 unquantised from 7 bits
}

TEST(Fs2024Bc7, ZeroModeByteIsTransparentBlack) {
    BitWriter w;
    const auto px = Decode(w);
    for (std::uint8_t v : px) EXPECT_EQ(v, 0);
}

// Mode 4 is the one mode whose two index sets have different widths:
// colour indices are 2 bits and alpha indices 3, not the other way
// round. Reading them swapped decoded 4.5% of the blocks of FS2024's
// own texture array wrongly -- every block that uses mode 4.
TEST(Fs2024Bc7, Mode4ColourIndicesAreTwoBitsAlphaThree) {
    BitWriter w;
    w.Put(1 << 4, 5);                    // mode 4
    w.Put(0, 2);                         // no rotation
    w.Put(0, 1);                         // no index swap
    for (int v : {0, 31, 0, 31, 0, 31}) w.Put(v, 5);
    w.Put(0, 6); w.Put(63, 6);           // alpha endpoints
    w.Put(0, 1);                         // texel 0 colour index (anchor)
    w.Put(3, 2);                         // texel 1 colour index, max
    const auto px = Decode(w);
    EXPECT_EQ(px[4], 255);               // texel 1 took endpoint 1 whole
    EXPECT_EQ(px[5], 255);
    EXPECT_EQ(px[6], 255);
    EXPECT_EQ(px[7], 0);                 // its alpha index is still 0
}
