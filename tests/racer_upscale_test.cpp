#include "services/interfaces/workflow/racer/data/racer_upscale.hpp"

#include <array>

#include <gtest/gtest.h>

using sdl3cpp::services::impl::UpscaleRgba;
using sdl3cpp::services::impl::UpscaleScale2x;

namespace {

using Rgba = std::array<std::uint8_t, 4>;

void SetPixel(std::vector<std::uint8_t>& image, int width, int x, int y,
              Rgba colour) {
    const std::size_t at = 4 * (static_cast<std::size_t>(y) * width + x);
    for (std::size_t c = 0; c < 4; ++c) image[at + c] = colour[c];
}

Rgba GetPixel(const std::vector<std::uint8_t>& image, int width, int x,
              int y) {
    const std::size_t at = 4 * (static_cast<std::size_t>(y) * width + x);
    return {image[at], image[at + 1], image[at + 2], image[at + 3]};
}

const Rgba kBlue{0, 0, 255, 255};
const Rgba kRed{255, 0, 0, 255};
const Rgba kGreen{0, 255, 0, 255};

}  // namespace

TEST(RacerUpscale, UniformImageStaysUniform) {
    const std::vector<std::uint8_t> image(4 * 2 * 2, 77);
    const auto out = UpscaleScale2x(image, 2, 2);
    EXPECT_EQ(out.size(), 4u * 4u * 4u);
    for (std::uint8_t byte : out) EXPECT_EQ(byte, 77);
}

TEST(RacerUpscale, Scale2xSharpensDiagonalCorner) {
    // Centre red; left and up blue; right and down green.
    std::vector<std::uint8_t> image(4 * 3 * 3, 0);
    for (int i = 0; i < 9; ++i) SetPixel(image, 3, i % 3, i / 3, kBlue);
    SetPixel(image, 3, 1, 1, kRed);
    SetPixel(image, 3, 2, 1, kGreen);
    SetPixel(image, 3, 1, 2, kGreen);
    const auto out = UpscaleScale2x(image, 3, 3);
    // The centre pixel is output at (2,2)..(3,3).
    EXPECT_EQ(GetPixel(out, 6, 2, 2), kBlue);
    EXPECT_EQ(GetPixel(out, 6, 3, 2), kRed);
    EXPECT_EQ(GetPixel(out, 6, 2, 3), kRed);
    EXPECT_EQ(GetPixel(out, 6, 3, 3), kGreen);
}

TEST(RacerUpscale, FactorMustBePowerOfTwo) {
    const std::vector<std::uint8_t> image(4, 1);
    EXPECT_TRUE(UpscaleRgba(image, 1, 1, 3).empty());
    EXPECT_EQ(UpscaleRgba(image, 1, 1, 4).size(), 4u * 4u * 4u);
}
