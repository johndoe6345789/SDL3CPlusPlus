// The engine's tile grids must land exactly on FS2024's own quadtree at
// every streamed level, or every tile would fetch the wrong patch of the
// world's data: a key's tile has to be the quad tile its corner lies in.

#include "services/interfaces/workflow/fs2024/world/fs2024_geo_origin.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world_rebase.hpp"

#include <gtest/gtest.h>

#include <cmath>

namespace impl = sdl3cpp::services::impl;

TEST(Fs2024GeoOrigin, SpawnLiesInCoarsestTileZeroZero) {
    const auto origin = impl::MakeFs2024GeoOrigin(51.5007, -0.1246);
    float x = 0.f, z = 0.f;
    impl::Fs2024EngineOfLatLon(origin, 51.5007, -0.1246, x, z);
    const float coarsest = impl::Fs2024TileSpan(11, origin.TileSize());
    EXPECT_GE(x, 0.f);
    EXPECT_GE(z, 0.f);
    EXPECT_LT(x, coarsest);
    EXPECT_LT(z, coarsest);
    EXPECT_EQ(origin.tileX % 8, 0);
    EXPECT_EQ(origin.tileY % 8, 0);
    EXPECT_NEAR(origin.TileSize(), 1523.f, 5.f);  // level 14 at London
}

TEST(Fs2024GeoOrigin, EngineAndLatLonRoundTrip) {
    const auto origin = impl::MakeFs2024GeoOrigin(47.26, 11.39);
    for (const auto [lat, lon] : {std::pair{47.20, 11.30}, {47.35, 11.50}}) {
        float x = 0.f, z = 0.f;
        impl::Fs2024EngineOfLatLon(origin, lat, lon, x, z);
        double backLat = 0.0, backLon = 0.0;
        impl::Fs2024LatLonOfEngine(origin, x, z, backLat, backLon);
        EXPECT_NEAR(backLat, lat, 1e-6);
        EXPECT_NEAR(backLon, lon, 1e-6);
    }
}

TEST(Fs2024GeoOrigin, EngineGridKeysAreFs2024QuadTilesAtEveryLevel) {
    // Points on both sides of the origin: at every streamed level, a
    // point's engine key must name the quad tile the point falls in.
    const auto origin = impl::MakeFs2024GeoOrigin(51.5007, -0.1246);
    for (const auto [x, z] : {std::pair{5000.f, 3000.f}, {-21000.f, 700.f},
                              {33000.f, -18000.f}}) {
        double u = 0.0, v = 0.0;
        impl::Fs2024MercatorOfEngine(origin, x, z, u, v);
        for (int level = 11; level <= 14; ++level) {
            const auto key =
                impl::Fs2024TileKeyFor(x, z, origin.TileSize(), level);
            int quadX = 0, quadY = 0;
            impl::Fs2024QuadOfKey(origin, key, quadX, quadY);
            EXPECT_EQ(quadX, static_cast<int>(std::floor(u * (1 << level))));
            EXPECT_EQ(quadY, static_cast<int>(std::floor(v * (1 << level))));
        }
    }
}

TEST(Fs2024GeoOrigin, RebaseShiftsByWholeTilesAndKeepsTheScale) {
    // 60 km east of London: due for a re-base, which moves the origin by
    // whole coarsest tiles, keeps the scale -- so every tile built
    // already stays true, just moved -- and keeps the player on the
    // same spot of the Earth.
    auto origin = impl::MakeFs2024GeoOrigin(51.5007, -0.1246);
    const double scale = origin.metresPerUnit;
    const float x = 60000.f, z = -2500.f;
    ASSERT_TRUE(impl::Fs2024RebaseDue(x, z, 40000.f));
    double lat = 0.0, lon = 0.0;
    impl::Fs2024LatLonOfEngine(origin, x, z, lat, lon);

    const impl::Fs2024Rebase rebase = impl::RebaseFs2024Origin(origin, x, z);
    EXPECT_FALSE(rebase.rescaled);
    EXPECT_EQ(origin.metresPerUnit, scale);
    EXPECT_EQ(rebase.tilesX % 8, 0);
    EXPECT_EQ(rebase.tilesY % 8, 0);
    EXPECT_FLOAT_EQ(rebase.shift.x, rebase.tilesX * origin.TileSize());
    const glm::vec2 moved = glm::vec2(x, z) - rebase.shift;
    double newLat = 0.0, newLon = 0.0;
    impl::Fs2024LatLonOfEngine(origin, moved.x, moved.y, newLat, newLon);
    EXPECT_NEAR(newLat, lat, 1e-5);   // ~1 m
    EXPECT_NEAR(newLon, lon, 1e-5);
    EXPECT_GE(moved.x, 0.f);
    EXPECT_LT(moved.x, impl::Fs2024TileSpan(11, origin.TileSize()));
    EXPECT_FALSE(impl::Fs2024RebaseDue(moved.x, moved.y, 40000.f));
}

TEST(Fs2024GeoOrigin, RebaseFarNorthTakesTheNewLatitudesScale) {
    // A degree and more north the old scale is over 1% out: the origin is
    // made afresh, and the player still stands where they were.
    auto origin = impl::MakeFs2024GeoOrigin(51.5007, -0.1246);
    const double scale = origin.metresPerUnit;
    const float x = 0.f, z = -170000.f;  // ~1.5 degrees north
    double lat = 0.0, lon = 0.0;
    impl::Fs2024LatLonOfEngine(origin, x, z, lat, lon);
    const impl::Fs2024Rebase rebase = impl::RebaseFs2024Origin(origin, x, z);
    EXPECT_TRUE(rebase.rescaled);
    EXPECT_LT(origin.metresPerUnit, scale);
    const glm::vec2 moved = glm::vec2(x, z) - rebase.shift;
    double newLat = 0.0, newLon = 0.0;
    impl::Fs2024LatLonOfEngine(origin, moved.x, moved.y, newLat, newLon);
    EXPECT_NEAR(newLat, lat, 1e-5);
    EXPECT_NEAR(newLon, lon, 1e-5);
}
