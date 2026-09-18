// The fs2024 tile streamer: key math across levels, the LOD cut, the
// plan that swaps detail without holes or overlaps, the resolve pass,
// and multi-tile height lookup. No GPU device is needed: none of them
// touch a tile's GPU handles.

#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_key.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_lookup.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_plan.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_resolve.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <unordered_set>
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

TEST(Fs2024TileKey, LevelsNestExactly) {
    using K = impl::Fs2024TileKey;
    EXPECT_EQ(impl::Fs2024TileParent({3, 2, 14}), (K{1, 1, 13}));
    EXPECT_EQ(impl::Fs2024TileParent({-1, -1, 14}), (K{-1, -1, 13}));
    EXPECT_TRUE(impl::Fs2024TileContains({1, 1, 13}, {3, 2, 14}));
    EXPECT_FALSE(impl::Fs2024TileContains({1, 1, 13}, {4, 2, 14}));
    EXPECT_TRUE(impl::Fs2024TileContains({-1, 0, 11}, {-5, 7, 14}));
    EXPECT_TRUE(impl::Fs2024TileContains({2, 2, 12}, {2, 2, 12}));
    EXPECT_FALSE(impl::Fs2024TileContains({3, 2, 14}, {1, 1, 13}));
}

TEST(Fs2024TileKey, EveryLevelsCellHoldsItsPoint) {
    for (const auto [x, z] : {std::pair{500.f, 500.f}, {-2500.f, 9100.f}}) {
        for (int level = 11; level <= 14; ++level) {
            const auto key = impl::Fs2024TileKeyFor(x, z, 1000.f, level);
            const glm::vec3 corner = impl::Fs2024TileCorner(key, 1000.f);
            const float span = impl::Fs2024TileSpan(level, 1000.f);
            EXPECT_EQ(span, 1000.f * static_cast<float>(1 << (14 - level)));
            EXPECT_LE(corner.x, x);
            EXPECT_LT(x, corner.x + span);
            EXPECT_LE(corner.z, z);
            EXPECT_LT(z, corner.z + span);
        }
    }
}

TEST(Fs2024TileLod, FinestUnderfootCoarsestAtTheEdge) {
    const glm::vec3 viewer(500.f, 0.f, 500.f);
    const auto leaves = impl::SelectFs2024Tiles(viewer, 1000.f, {});
    ASSERT_FALSE(leaves.empty());
    EXPECT_EQ(leaves.front(), (impl::Fs2024TileKey{0, 0, 14}));
    EXPECT_EQ(leaves.back().level, 11);
    for (std::size_t i = 1; i < leaves.size(); ++i) {
        EXPECT_LE(impl::Fs2024TileDistance(leaves[i - 1], 1000.f, viewer),
                  impl::Fs2024TileDistance(leaves[i], 1000.f, viewer));
    }
}

TEST(Fs2024TileLod, LeavesCoverTheRootSquareExactlyOnce) {
    const glm::vec3 viewer(-3700.f, 0.f, 12900.f);
    const auto leaves = impl::SelectFs2024Tiles(viewer, 1000.f, {});
    double area = 0.0;
    for (const auto& leaf : leaves) {
        const double span = impl::Fs2024TileSpan(leaf.level, 1000.f);
        area += span * span;
        for (const auto& other : leaves) {
            if (!(other == leaf)) {
                EXPECT_FALSE(impl::Fs2024TileContains(leaf, other));
            }
        }
    }
    const double root = 5 * impl::Fs2024TileSpan(11, 1000.f);  // radius 2
    EXPECT_DOUBLE_EQ(area, root * root);
}

TEST(Fs2024TileLod, ClimbingCoarsensTheGroundBelow) {
    const auto finest = [](float height) {
        const auto leaves =
            impl::SelectFs2024Tiles({500.f, height, 500.f}, 1000.f, {});
        return std::count_if(leaves.begin(), leaves.end(),
                             [](const auto& k) { return k.level == 14; });
    };
    EXPECT_GT(finest(0.f), finest(1500.f));
    EXPECT_EQ(finest(3000.f), 0);
}

namespace {

std::unordered_set<impl::Fs2024TileKey> Keys(
    std::initializer_list<impl::Fs2024TileKey> keys) {
    return {keys.begin(), keys.end()};
}

const std::vector<impl::Fs2024TileKey> kChildren = {
    {2, 2, 14}, {3, 2, 14}, {2, 3, 14}, {3, 3, 14}};
const impl::Fs2024TileKey kParent{1, 1, 13};

}  // namespace

TEST(Fs2024TilePlan, ParentStaysUntilEveryChildIsReady) {
    auto plan = impl::PlanFs2024Tiles(kChildren, Keys({kParent}), {});
    EXPECT_EQ(plan.load.size(), 4u);
    EXPECT_EQ(plan.draw, Keys({kParent}));
    EXPECT_TRUE(plan.evict.empty());

    // Three of four in: still only the parent, never both over one spot.
    plan = impl::PlanFs2024Tiles(
        kChildren, Keys({kParent, kChildren[0], kChildren[1], kChildren[2]}),
        {});
    EXPECT_EQ(plan.draw, Keys({kParent}));
    EXPECT_TRUE(plan.evict.empty());

    plan = impl::PlanFs2024Tiles(
        kChildren, Keys({kParent, kChildren[0], kChildren[1], kChildren[2],
                         kChildren[3]}),
        {});
    EXPECT_EQ(plan.draw.size(), 4u);
    EXPECT_EQ(plan.evict, (std::vector<impl::Fs2024TileKey>{kParent}));
}

TEST(Fs2024TilePlan, ChildrenStayUntilTheirParentIsReady) {
    const auto children = Keys({kChildren[0], kChildren[1], kChildren[2],
                                kChildren[3]});
    auto plan = impl::PlanFs2024Tiles({kParent}, children, {});
    EXPECT_EQ(plan.load, (std::vector<impl::Fs2024TileKey>{kParent}));
    EXPECT_EQ(plan.draw, children);
    EXPECT_TRUE(plan.evict.empty());

    auto both = children;
    both.insert(kParent);
    plan = impl::PlanFs2024Tiles({kParent}, both, {});
    EXPECT_EQ(plan.draw, Keys({kParent}));
    EXPECT_EQ(plan.evict.size(), 4u);
}

TEST(Fs2024TilePlan, AFailedTileIsNeitherLoadedNorDrawn) {
    const auto plan =
        impl::PlanFs2024Tiles({kParent}, {}, Keys({kParent}));
    EXPECT_TRUE(plan.load.empty());
    EXPECT_TRUE(plan.draw.empty());
}

TEST(Fs2024TilesResolve, SkipsTilesAlreadyLoadingAndForgetsOldFailures) {
    impl::Fs2024TileStreamState state;
    state.tileSize = 1000.f;
    state.loading.insert({0, 0, 14});
    state.missing.insert({900, 900, 14});  // far away: forgotten
    impl::Fs2024ResolveWantedTiles(state, {500.f, 0.f, 500.f});
    EXPECT_FALSE(state.pendingLoad.empty());
    EXPECT_EQ(std::count(state.pendingLoad.begin(), state.pendingLoad.end(),
                         impl::Fs2024TileKey{0, 0, 14}),
              0);
    EXPECT_TRUE(state.missing.empty());
    EXPECT_EQ(state.wanted.size(), state.pendingLoad.size() + 1);
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

TEST(Fs2024TileLookup, PrefersTheFinestResidentTile) {
    impl::Fs2024TileStreamState state;
    state.tileSize = 1000.f;
    impl::Fs2024LoadedTile coarse = FlatTile(0, 0, 8000.f, 99.f);
    state.resident.emplace(impl::Fs2024TileKey{0, 0, 11}, std::move(coarse));
    state.resident.emplace(impl::Fs2024TileKey{0, 0, 14},
                           FlatTile(0, 0, 1000.f, 10.f));
    const auto* near = impl::Fs2024FindTileField(state, 500.f, 500.f);
    ASSERT_NE(near, nullptr);
    EXPECT_FLOAT_EQ(impl::Fs2024HeightAt(*near, 500.f, 500.f), 10.f);
    const auto* far = impl::Fs2024FindTileField(state, 5500.f, 500.f);
    ASSERT_NE(far, nullptr);
    EXPECT_FLOAT_EQ(impl::Fs2024HeightAt(*far, 5500.f, 500.f), 99.f);
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
