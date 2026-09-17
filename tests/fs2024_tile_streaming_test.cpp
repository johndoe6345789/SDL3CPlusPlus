// The fs2024 tile streamer: key math, the resolve pass that decides
// what to load/evict, and multi-tile height lookup. No GPU device is
// needed since resolve/height-lookup never touch a tile's GPU handles.

#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_key.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_lookup.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_resolve.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <utility>

namespace impl = sdl3cpp::services::impl;

namespace {

/// A tile whose whole field sits at `height`, for lookup tests.
impl::Fs2024LoadedTile FlatTile(int tx, int tz, float tileSize,
                                float height) {
    impl::Fs2024LoadedTile tile;
    tile.terrain.field.columns = 3;
    tile.terrain.field.rows = 3;
    tile.terrain.field.spacing = tileSize / 2.f;
    tile.terrain.field.origin = {tx * tileSize, tz * tileSize};
    tile.terrain.field.heights = {height, height, height, height,
                                  height, height, height, height, height};
    tile.terrain.field.minHeight = tile.terrain.field.maxHeight = height;
    tile.terrain.loaded = true;
    return tile;
}

int CountPending(const std::vector<impl::Fs2024TileKey>& list, int x,
                 int z) {
    return static_cast<int>(std::count_if(
        list.begin(), list.end(), [&](const impl::Fs2024TileKey& k) {
            return k.x == x && k.z == z;
        }));
}

}  // namespace

TEST(Fs2024TileKey, FloorsTowardsNegativeInfinity) {
    EXPECT_EQ((impl::Fs2024TileKeyFor(999.f, -1.f, 1000.f)),
             (impl::Fs2024TileKey{0, -1}));
    EXPECT_EQ((impl::Fs2024TileKeyFor(-0.001f, 0.f, 1000.f)),
             (impl::Fs2024TileKey{-1, 0}));
    EXPECT_EQ((impl::Fs2024TileKeyFor(1000.f, 1000.f, 1000.f)),
             (impl::Fs2024TileKey{1, 1}));
}

TEST(Fs2024TileKey, RingIsChebyshevDistance) {
    EXPECT_EQ(impl::Fs2024TileRing({5, 5}, {5, 5}), 0);
    EXPECT_EQ(impl::Fs2024TileRing({7, 5}, {5, 5}), 2);
    EXPECT_EQ(impl::Fs2024TileRing({5, 8}, {5, 5}), 3);
    EXPECT_EQ(impl::Fs2024TileRing({-2, -2}, {0, 0}), 2);
}

TEST(Fs2024TileKey, DirectoryMatchesTheBakeLayout) {
    EXPECT_EQ(impl::Fs2024TileDirectory("D:/fs2024/westminster", {-3, 2}),
             "D:/fs2024/westminster/tiles/-3_2");
}

TEST(Fs2024TilesResolve, WantsASquareOfTilesAroundThePlayer) {
    impl::Fs2024TileStreamState state;
    state.tileSize = 1000.f;
    state.loadRadiusTiles = 1;
    state.evictRadiusTiles = 2;

    Fs2024ResolveWantedTiles(state, 500.f, 500.f);  // centre tile (0, 0)

    EXPECT_EQ(state.pendingLoad.size(), 9u);
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dx = -1; dx <= 1; ++dx) {
            EXPECT_EQ(CountPending(state.pendingLoad, dx, dz), 1)
                << dx << ", " << dz;
        }
    }
    EXPECT_TRUE(state.pendingEvict.empty());
}

TEST(Fs2024TilesResolve, NearestTilesComeFirst) {
    impl::Fs2024TileStreamState state;
    state.tileSize = 1000.f;
    state.loadRadiusTiles = 2;

    Fs2024ResolveWantedTiles(state, 0.f, 0.f);

    ASSERT_FALSE(state.pendingLoad.empty());
    EXPECT_EQ(state.pendingLoad.front(), (impl::Fs2024TileKey{0, 0}));
    for (std::size_t i = 1; i < state.pendingLoad.size(); ++i) {
        EXPECT_LE(impl::Fs2024TileRing(state.pendingLoad[i - 1], {0, 0}),
                  impl::Fs2024TileRing(state.pendingLoad[i], {0, 0}));
    }
}

TEST(Fs2024TilesResolve, DoesNotReQueueAResidentOrAlreadyPendingTile) {
    impl::Fs2024TileStreamState state;
    state.tileSize = 1000.f;
    state.loadRadiusTiles = 1;
    state.resident.emplace(impl::Fs2024TileKey{0, 0}, FlatTile(0, 0, 1000.f,
                                                               0.f));

    Fs2024ResolveWantedTiles(state, 0.f, 0.f);
    EXPECT_EQ(CountPending(state.pendingLoad, 0, 0), 0);
    EXPECT_EQ(state.pendingLoad.size(), 8u);

    // A second resolve before anything loads must not duplicate them.
    Fs2024ResolveWantedTiles(state, 0.f, 0.f);
    EXPECT_EQ(state.pendingLoad.size(), 8u);
}

TEST(Fs2024TilesResolve, EvictsOnlyBeyondTheEvictRadiusNotTheLoadRadius) {
    impl::Fs2024TileStreamState state;
    state.tileSize = 1000.f;
    state.loadRadiusTiles = 1;
    state.evictRadiusTiles = 3;
    state.resident.emplace(impl::Fs2024TileKey{2, 0},
                           FlatTile(2, 0, 1000.f, 0.f));  // ring 2: kept
    state.resident.emplace(impl::Fs2024TileKey{4, 0},
                           FlatTile(4, 0, 1000.f, 0.f));  // ring 4: evicted

    Fs2024ResolveWantedTiles(state, 0.f, 0.f);

    EXPECT_EQ(state.pendingEvict.size(), 1u);
    EXPECT_EQ(state.pendingEvict.front(), (impl::Fs2024TileKey{4, 0}));
}

TEST(Fs2024TilesResolve, NeverWantsATileAlreadyMarkedMissing) {
    impl::Fs2024TileStreamState state;
    state.tileSize = 1000.f;
    state.loadRadiusTiles = 1;
    state.missing.insert({1, 0});

    Fs2024ResolveWantedTiles(state, 0.f, 0.f);
    EXPECT_EQ(CountPending(state.pendingLoad, 1, 0), 0);
}

TEST(Fs2024TileLookup, FindsTheFieldOfTheTileThatActuallyContainsThePoint) {
    impl::Fs2024TileStreamState state;
    state.tileSize = 1000.f;
    state.resident.emplace(impl::Fs2024TileKey{0, 0},
                           FlatTile(0, 0, 1000.f, 10.f));
    state.resident.emplace(impl::Fs2024TileKey{1, 0},
                           FlatTile(1, 0, 1000.f, 50.f));

    const auto* here = impl::Fs2024FindTileField(state, 500.f, 500.f);
    ASSERT_NE(here, nullptr);
    EXPECT_FLOAT_EQ(impl::Fs2024HeightAt(*here, 500.f, 500.f), 10.f);

    const auto* next = impl::Fs2024FindTileField(state, 1500.f, 500.f);
    ASSERT_NE(next, nullptr);
    EXPECT_FLOAT_EQ(impl::Fs2024HeightAt(*next, 1500.f, 500.f), 50.f);
}

TEST(Fs2024TileLookup, ReturnsNullOverAnUnloadedTile) {
    impl::Fs2024TileStreamState state;
    state.tileSize = 1000.f;
    EXPECT_EQ(impl::Fs2024FindTileField(state, 500.f, 500.f), nullptr);
}

TEST(Fs2024TileLookup, IgnoresATileThatIsResidentButNotYetLoaded) {
    impl::Fs2024TileStreamState state;
    state.tileSize = 1000.f;
    impl::Fs2024LoadedTile half = FlatTile(0, 0, 1000.f, 10.f);
    half.terrain.loaded = false;  // e.g. still mid-upload
    state.resident.emplace(impl::Fs2024TileKey{0, 0}, std::move(half));
    EXPECT_EQ(impl::Fs2024FindTileField(state, 500.f, 500.f), nullptr);
}
