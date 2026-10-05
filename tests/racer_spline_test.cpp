#include "services/interfaces/workflow/racer/data/racer_spline.hpp"

#include <cstring>
#include <vector>

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;

namespace {

void PutBe16(std::vector<std::uint8_t>& out, std::size_t at, int value) {
    out[at] = static_cast<std::uint8_t>(value >> 8);
    out[at + 1] = static_cast<std::uint8_t>(value);
}

void PutBeFloat(std::vector<std::uint8_t>& out, std::size_t at, float v) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &v, 4);
    for (int k = 0; k < 4; ++k) {
        out[at + k] = static_cast<std::uint8_t>(bits >> (24 - 8 * k));
    }
}

/// Three segments in a ring 0 -> 1 -> 2 -> 0, knots at x = 0, 10, 20.
std::vector<std::uint8_t> RingSpline() {
    std::vector<std::uint8_t> item(16 + 3 * 84, 0x20);
    PutBe16(item, 4, 0);
    PutBe16(item, 6, 3);  // segment count at +4 (u32)
    for (int i = 0; i < 3; ++i) {
        const std::size_t at = 16 + 84 * i;
        PutBe16(item, at, 1);                // one predecessor
        PutBe16(item, at + 2, 1);            // one successor
        PutBe16(item, at + 4, (i + 1) % 3);  // successor 0
        PutBe16(item, at + 8, (i + 2) % 3);  // predecessor 0
        for (int k = 0; k < 3; ++k) {
            PutBeFloat(item, at + 0x10 + 4 * k, k == 0 ? 10.f * i : 0.f);
            PutBeFloat(item, at + 0x28 + 4 * k, k == 0 ? 10.f * i : 0.f);
            PutBeFloat(item, at + 0x34 + 4 * k, k == 0 ? 10.f * i : 0.f);
        }
    }
    return item;
}

}  // namespace

TEST(RacerSpline, ReadsLinksAndKnots) {
    const auto segments = ReadRacerSpline(RingSpline());
    ASSERT_EQ(segments.size(), 3u);
    EXPECT_EQ(segments[1].successors[0], 2);
    EXPECT_EQ(segments[1].successors[1], -1);  // beyond the count
    EXPECT_EQ(segments[1].predecessors[0], 0);
    EXPECT_FLOAT_EQ(segments[2].knot.x, 20.f);
}

TEST(RacerSpline, MainLoopFollowsFirstSuccessors) {
    const auto loop = RacerSplineMainLoop(ReadRacerSpline(RingSpline()));
    EXPECT_EQ(loop, (std::vector<int>{0, 1, 2}));
}

TEST(RacerSpline, BezierStartsAndEndsOnKnots) {
    const auto s = ReadRacerSpline(RingSpline());
    EXPECT_FLOAT_EQ(RacerSplinePoint(s[0], s[1], 0.f).x, 0.f);
    EXPECT_FLOAT_EQ(RacerSplinePoint(s[0], s[1], 1.f).x, 10.f);
}

TEST(RacerSpline, RejectsShortItem) {
    std::vector<std::uint8_t> item(40, 0);
    PutBe16(item, 6, 1);
    EXPECT_TRUE(ReadRacerSpline(item).empty());
}
