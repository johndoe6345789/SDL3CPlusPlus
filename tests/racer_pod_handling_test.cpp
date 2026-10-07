#include "services/interfaces/workflow/racer/player/racer_pod_handling.hpp"

#include <gtest/gtest.h>

using sdl3cpp::services::impl::RacerHandlingRecord;
using sdl3cpp::services::impl::RacerSpecFromHandling;

namespace {

// Anakin's and Sebulba's rows of the game's pod handling table.
const RacerHandlingRecord kAnakin{0.5f, 300, 110, 3, 490, 30, 60, 200,
                                  13,   9,   4.99f, 0.4f, 50, 0.6f, 5};
const RacerHandlingRecord kSebulba{0.38f, 228, 95, 3.2f, 600, 38, 50, 185,
                                   9,     2,   4.99f, 0.19f, 80, 0.3f, 7};

}  // namespace

TEST(RacerPodHandling, SpeedsReadAsKilometresPerHour) {
    const auto anakin = RacerSpecFromHandling(kAnakin);
    EXPECT_NEAR(anakin.topSpeed * 3.6f, 490.f, 0.1f);
    EXPECT_NEAR(anakin.boostSpeed * 3.6f, 690.f, 0.1f);
}

TEST(RacerPodHandling, SebulbaIsFasterHeavierAndRunsHotter) {
    const auto anakin = RacerSpecFromHandling(kAnakin);
    const auto sebulba = RacerSpecFromHandling(kSebulba);
    EXPECT_GT(sebulba.topSpeed, anakin.topSpeed);
    EXPECT_GT(sebulba.bumpMass, anakin.bumpMass);
    EXPECT_LT(sebulba.coolRate, anakin.coolRate);
    EXPECT_LT(sebulba.turnRate, anakin.turnRate);
}

TEST(RacerPodHandling, AnEmptyRecordKeepsTheStockPod) {
    const auto spec = RacerSpecFromHandling(RacerHandlingRecord{});
    EXPECT_GT(spec.topSpeed, 100.f);
}
