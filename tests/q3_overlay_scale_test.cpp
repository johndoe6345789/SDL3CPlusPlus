// Overlay rasterisation size. ioq3 keeps 640x480 coordinates but draws
// at native resolution; rendering into a 640x480 surface and stretching
// it is what makes the menu soft on a modern display.

#include "services/interfaces/workflow/quake3/q3_overlay_scale.hpp"

#include <gtest/gtest.h>

using sdl3cpp::q3::ChooseOverlaySize;
using sdl3cpp::q3::kMaxOverlayScale;

TEST(ChooseOverlaySize, NativeResolutionGivesOneToOne) {
    const auto size = ChooseOverlaySize(640, 480);
    EXPECT_EQ(size.scale, 1);
    EXPECT_EQ(size.width, 640);
    EXPECT_EQ(size.height, 480);
}

TEST(ChooseOverlaySize, DoublesForTheDefaultWindow) {
    const auto size = ChooseOverlaySize(1280, 960);
    EXPECT_EQ(size.scale, 2);
    EXPECT_EQ(size.width, 1280);
    EXPECT_EQ(size.height, 960);
}

TEST(ChooseOverlaySize, TheSmallerAxisDecides) {
    // A wide window must not rasterise taller than its own height.
    const auto size = ChooseOverlaySize(2560, 1000);
    EXPECT_EQ(size.scale, 2);
    EXPECT_LE(size.height, 1000);
}

TEST(ChooseOverlaySize, KeepsWholePixelMultiples) {
    const auto size = ChooseOverlaySize(1600, 1100);
    EXPECT_EQ(size.width % 640, 0);
    EXPECT_EQ(size.height % 480, 0);
}

TEST(ChooseOverlaySize, ClampsSoLargeDisplaysStayAffordable) {
    const auto size = ChooseOverlaySize(7680, 4320);
    EXPECT_EQ(size.scale, kMaxOverlayScale);
}

TEST(ChooseOverlaySize, NeverGoesBelowOneOnSmallWindows) {
    const auto size = ChooseOverlaySize(320, 200);
    EXPECT_EQ(size.scale, 1);
    EXPECT_EQ(size.width, 640);
}

TEST(ChooseOverlaySize, SurvivesDegenerateSizes) {
    for (const auto& [w, h] : {std::pair{0, 0}, {-1, 480}, {640, -1}}) {
        const auto size = ChooseOverlaySize(w, h);
        EXPECT_GE(size.scale, 1);
        EXPECT_EQ(size.width, 640 * size.scale);
    }
}

TEST(ChooseOverlaySize, AspectRatioIsAlwaysFourThirds) {
    for (const int w : {640, 1280, 1920, 2560, 3840}) {
        const auto size = ChooseOverlaySize(w, w * 3 / 4);
        EXPECT_EQ(size.width * 3, size.height * 4);
    }
}
