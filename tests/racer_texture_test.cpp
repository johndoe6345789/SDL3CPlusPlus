#include "services/interfaces/workflow/racer/data/racer_asset_library.hpp"
#include "services/interfaces/workflow/racer/data/racer_texture.hpp"

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;

TEST(RacerTexture, Argb1555ExpandsEachChannel) {
    // Red is bits 11-15, green 6-10, blue 1-5, alpha bit 0.
    const std::vector<std::uint8_t> raw = {0xF8, 0x00, 0x07, 0xC0,
                                           0x00, 0x1E, 0x00, 0x01};
    const auto palette = DecodeRacerPalette(raw, 4);
    ASSERT_EQ(palette.size(), 4u);
    EXPECT_EQ(palette[0][0], 255);  // red of 0xF800
    EXPECT_EQ(palette[0][3], 0);    // alpha bit clear
    EXPECT_EQ(palette[1][1], 255);  // green of 0x07C0
    EXPECT_EQ(palette[2][2], 255 * 15 / 31);
    EXPECT_EQ(palette[3][3], 255);  // alpha bit set
}

TEST(RacerTexture, Indexed4ReadsHighNibbleFirst) {
    // Palette: 0 = opaque blue (0x003F), 1 = opaque red (0xF801).
    const std::vector<std::uint8_t> palette = {0x00, 0x3F, 0xF8, 0x01};
    const std::vector<std::uint8_t> pixels = {0x10};
    const auto t = DecodeRacerTexture(pixels, palette,
                                      RacerTextureFormat::Indexed4, 2, 1);
    ASSERT_EQ(t.rgba.size(), 8u);
    EXPECT_EQ(t.rgba[0], 255);  // pixel 0 red
    EXPECT_EQ(t.rgba[2], 0);
    EXPECT_EQ(t.rgba[4], 0);    // pixel 1 blue
    EXPECT_EQ(t.rgba[6], 255);
}

TEST(RacerTexture, Intensity8CopiesGreyIntoAlpha) {
    const auto t = DecodeRacerTexture({0x80}, {},
                                      RacerTextureFormat::Intensity8, 1, 1);
    ASSERT_EQ(t.rgba.size(), 4u);
    EXPECT_EQ(t.rgba[0], 0x80);
    EXPECT_EQ(t.rgba[3], 0x80);
}

TEST(RacerTexture, ShortBufferShowsMagenta) {
    const auto t = DecodeRacerTexture({}, {}, RacerTextureFormat::Rgba32,
                                      1, 1);
    ASSERT_EQ(t.rgba.size(), 4u);
    EXPECT_EQ(t.rgba[0], 255);
    EXPECT_EQ(t.rgba[1], 0);
    EXPECT_EQ(t.rgba[2], 255);
}

TEST(RacerTexture, MirrorReflectsTheRightHalf) {
    RacerTexture t;
    t.width = 2;
    t.height = 1;
    t.rgba = {1, 1, 1, 1, 2, 2, 2, 2};
    const auto m = MirrorRacerTexture(t, true, false);
    ASSERT_EQ(m.width, 4);
    const std::vector<std::uint8_t> row = {m.rgba[0], m.rgba[4], m.rgba[8],
                                           m.rgba[12]};
    EXPECT_EQ(row, (std::vector<std::uint8_t>{1, 2, 2, 1}));
}
