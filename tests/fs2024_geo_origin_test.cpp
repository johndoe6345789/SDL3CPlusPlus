// The engine's tile grid must land exactly on FS2024's own level-14
// quadtree, or every tile would fetch the wrong patch of the world's
// data: a key's tile has to be the quad tile its corner lies in.

#include "services/interfaces/workflow/fs2024/world/fs2024_geo_origin.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world_rebase.hpp"

#include <gtest/gtest.h>

#include <cmath>

namespace impl = sdl3cpp::services::impl;

TEST(Fs2024GeoOrigin, SpawnLiesInTileZeroZero) {
    const auto origin = impl::MakeFs2024GeoOrigin(51.5007, -0.1246);
    float x = 0.f, z = 0.f;
    impl::Fs2024EngineOfLatLon(origin, 51.5007, -0.1246, x, z);
    EXPECT_GE(x, 0.f);
    EXPECT_GE(z, 0.f);
    EXPECT_LT(x, origin.TileSize());
    EXPECT_LT(z, origin.TileSize());
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

TEST(Fs2024GeoOrigin, EngineGridKeysAreFs2024QuadTiles) {
    // A point 5 km east and 3 km south: its engine key must name the
    // same quad tile the point itself falls in.
    const auto origin = impl::MakeFs2024GeoOrigin(51.5007, -0.1246);
    const float x = 5000.f, z = 3000.f;
    const impl::Fs2024TileKey key =
        impl::Fs2024TileKeyFor(x, z, origin.TileSize());
    int quadX = 0, quadY = 0;
    impl::Fs2024QuadOfKey(origin, key, quadX, quadY);
    double u = 0.0, v = 0.0;
    impl::Fs2024MercatorOfEngine(origin, x, z, u, v);
    EXPECT_EQ(quadX, static_cast<int>(std::floor(u * (1 << 14))));
    EXPECT_EQ(quadY, static_cast<int>(std::floor(v * (1 << 14))));
}

TEST(Fs2024GeoOrigin, RebaseKeepsThePlayerOnTheSameSpotOfTheEarth) {
    // 60 km east of London: due for a re-base, and after it the player
    // must stand on the same lat/lon, now in the new origin's tile 0.
    auto origin = impl::MakeFs2024GeoOrigin(51.5007, -0.1246);
    const float x = 60000.f, z = -2500.f;
    ASSERT_TRUE(impl::Fs2024RebaseDue(x, z, 40000.f));
    double lat = 0.0, lon = 0.0;
    impl::Fs2024LatLonOfEngine(origin, x, z, lat, lon);

    const glm::vec2 moved = impl::RebaseFs2024Origin(origin, x, z);
    double newLat = 0.0, newLon = 0.0;
    impl::Fs2024LatLonOfEngine(origin, moved.x, moved.y, newLat, newLon);
    EXPECT_NEAR(newLat, lat, 1e-5);   // ~1 m
    EXPECT_NEAR(newLon, lon, 1e-5);
    EXPECT_GE(moved.x, 0.f);
    EXPECT_LT(moved.x, origin.TileSize());
    EXPECT_FALSE(impl::Fs2024RebaseDue(moved.x, moved.y, 40000.f));
}
