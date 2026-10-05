#include "services/interfaces/workflow/racer/data/racer_spline.hpp"

#include <cstring>
#include <vector>

#include <gtest/gtest.h>

using sdl3cpp::services::impl::ReadRacerSpline;

namespace {

void PutBe32(std::vector<std::uint8_t>& out, std::size_t at,
             std::uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8) {
        out[at++] = static_cast<std::uint8_t>(value >> shift);
    }
}

void PutBeFloat(std::vector<std::uint8_t>& out, std::size_t at, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    PutBe32(out, at, bits);
}

}  // namespace

TEST(RacerSpline, ReadsRingIdsAndPointOrder) {
    // Header (count 1), then one 84-byte record with id 2, prev 1.
    std::vector<std::uint8_t> entry(84, 0);
    PutBe32(entry, 4, 1);
    PutBe32(entry, 20, 0x00020000);
    PutBe32(entry, 24, 0x00010000);
    PutBeFloat(entry, 32, 1.5f);   // x of point 0
    PutBeFloat(entry, 36, 2.5f);   // z of point 0
    PutBeFloat(entry, 40, 3.5f);   // height of point 0
    const auto records = ReadRacerSpline(entry.data(), entry.size());
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].id, 2u);
    EXPECT_EQ(records[0].prevId, 1u);
    EXPECT_FLOAT_EQ(records[0].points[0].x, 1.5f);
    EXPECT_FLOAT_EQ(records[0].points[0].y, 3.5f);
    EXPECT_FLOAT_EQ(records[0].points[0].z, 2.5f);
}

TEST(RacerSpline, RejectsEntryShorterThanItsCount) {
    std::vector<std::uint8_t> entry(40, 0);
    PutBe32(entry, 4, 1);
    EXPECT_TRUE(ReadRacerSpline(entry.data(), entry.size()).empty());
}
