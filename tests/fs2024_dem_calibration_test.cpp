// Fs2024DemMetres on its own: the curve must be strictly increasing
// across the whole int16 range (a flat or reversed step would terrace
// or fold real terrain), hold sea level at sea level, and reach the
// world's extremes.

#include "services/interfaces/workflow/fs2024/data/dem/fs2024_dem_calibration.hpp"

#include <gtest/gtest.h>

#include <limits>

namespace f = sdl3cpp::fs2024;

TEST(Fs2024DemCalibration, StrictlyIncreasingOverEveryRawValue) {
    float previous = f::Fs2024DemMetres(std::numeric_limits<std::int16_t>::min());
    for (int raw = std::numeric_limits<std::int16_t>::min() + 1;
         raw <= std::numeric_limits<std::int16_t>::max(); ++raw) {
        const float metres = f::Fs2024DemMetres(static_cast<std::int16_t>(raw));
        ASSERT_GT(metres, previous) << "at raw " << raw;
        previous = metres;
    }
}

TEST(Fs2024DemCalibration, SeaLevelIsSeaLevel) {
    // London's Thames sits at raw ~-22,500 in FS2024's own tiles.
    EXPECT_NEAR(f::Fs2024DemMetres(-22480), 0.f, 3.f);
}

TEST(Fs2024DemCalibration, ReachesTheWorldsExtremes) {
    EXPECT_LT(f::Fs2024DemMetres(-30840), -400.f);  // Dead Sea surface
    EXPECT_GT(f::Fs2024DemMetres(32470), 8400.f);   // Everest's tile top
}
