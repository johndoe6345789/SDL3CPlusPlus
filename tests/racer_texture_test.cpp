#include "services/interfaces/workflow/racer/data/racer_texture.hpp"

#include <gtest/gtest.h>

using sdl3cpp::services::impl::DecodeRacerPalette;
using sdl3cpp::services::impl::DecodeRacerTextures;

namespace {

/// Palette entry 0 is blue (0x003E), entry 1 is red (0xF800); the rest
/// are zero. The texture is then 0x10 everywhere: high nibble 1 (red),
/// low nibble 0 (blue).
std::vector<std::uint8_t> PaletteAndTextureBlock() {
    std::vector<std::uint8_t> block = {0, 0, 0, 2,   // count
                                       0, 0, 0, 12,  // palette at 12
                                       0, 0, 0, 44}; // texture at 44
    block.resize(12 + 32, 0);
    block[12 + 0] = 0x00;
    block[12 + 1] = 0x3E;  // palette entry 0: blue (ARGB1555)
    block[12 + 2] = 0xF8;
    block[12 + 3] = 0x00;  // palette entry 1: red
    block.resize(12 + 32 + 2048, 0x10);
    return block;
}

}  // namespace

TEST(RacerTexture, Argb1555ExpandsEachChannel) {
    // Red is bits 11-15, green 6-10, blue 1-5, alpha bit 0.
    // 0xF800: red 31, alpha 0. 0x07C0: green 31, alpha 0.
    // 0x001E: blue 15, alpha 0. 0x0001: alpha 1 only.
    const std::uint8_t raw[32] = {0xF8, 0x00, 0x07, 0xC0,
                                  0x00, 0x1E, 0x00, 0x01};
    const auto palette = DecodeRacerPalette(raw);
    EXPECT_EQ(palette[4 * 0 + 0], 255);  // red of 0xF800
    EXPECT_EQ(palette[4 * 0 + 3], 0);    // alpha bit clear
    EXPECT_EQ(palette[4 * 1 + 1], 255);  // green of 0x07C0
    EXPECT_EQ(palette[4 * 2 + 2], 255 * 15 / 31);  // blue of 0x001E
    EXPECT_EQ(palette[4 * 3 + 2], 0);    // blue of 0x0001
    EXPECT_EQ(palette[4 * 3 + 3], 255);  // alpha bit set
}

TEST(RacerTexture, PairsTextureWithPreceding32BytePalette) {
    const auto block = PaletteAndTextureBlock();
    const auto textures = DecodeRacerTextures(block);
    ASSERT_EQ(textures.size(), 1u);
    const auto& texture = textures[0];
    EXPECT_EQ(texture.width, 64);
    EXPECT_EQ(texture.height, 64);
    ASSERT_EQ(texture.rgba.size(), 64u * 64u * 4u);
    // Pixel 0: high nibble 1 -> red.
    EXPECT_EQ(texture.rgba[0], 255);
    EXPECT_EQ(texture.rgba[1], 0);
    EXPECT_EQ(texture.rgba[2], 0);
    // Pixel 1: low nibble 0 -> blue.
    EXPECT_EQ(texture.rgba[4 + 0], 0);
    EXPECT_EQ(texture.rgba[4 + 2], 255);
}

TEST(RacerTexture, SkipsTextureWithoutPalette) {
    std::vector<std::uint8_t> block = {0, 0, 0, 1, 0, 0, 0, 12};
    block.resize(12 + 2048, 0x10);
    EXPECT_TRUE(DecodeRacerTextures(block).empty());
}
