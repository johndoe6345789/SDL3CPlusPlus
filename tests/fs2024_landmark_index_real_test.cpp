// FS2024's own landmark placements against the installed game: every
// landmark model the game places, and where. Skipped when the game is
// absent.

#include "services/interfaces/workflow/fs2024/data/landmark/fs2024_landmark_index.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <set>

namespace f = sdl3cpp::fs2024;

namespace {

constexpr const char* kLibrary =
    "D:/Games/Official/Steam/fs-base/scenery/Global/Asobo_POI/Asobo_POI.BGL";
constexpr const char* kScenery =
    "D:/Games/Official/Steam/fs-base-genericairports/scenery";

double Metres(double lat1, double lon1, double lat2, double lon2) {
    const double k = 111320.0;
    return std::hypot((lon2 - lon1) * k * std::cos(lat1 * 3.14159265 / 180.0),
                      (lat2 - lat1) * k);
}

}  // namespace

TEST(Fs2024LandmarkIndexReal, FindsTheGamesOwnPlacementsWorldwide) {
    if (!std::filesystem::exists(kLibrary)) GTEST_SKIP() << "no FS2024";
    const auto placements = f::BuildLandmarkIndex(kLibrary, kScenery);
    std::set<std::string> models;
    for (const auto& p : placements) models.insert(p.model.name);
    std::printf("%zu placements of %zu models\n", placements.size(),
                models.size());
    EXPECT_GT(placements.size(), 1000u);
    EXPECT_GT(models.size(), 200u);

    // London's landmarks, where they really stand.
    for (const auto& p : placements) {
        if (Metres(p.lat, p.lon, 51.5007, -0.1246) < 6000.0) {
            std::printf("  London: %s at %.5f, %.5f hdg %.1f\n",
                        p.model.name.c_str(), p.lat, p.lon, p.headingDegrees);
        }
    }
    bool palace = false;
    for (const auto& p : placements) {
        if (p.model.name == "WestminsterPalace") {
            palace = true;
            EXPECT_LT(Metres(p.lat, p.lon, 51.4995, -0.1248), 150.0);
        }
    }
    EXPECT_TRUE(palace);
}
