// Walking on FS2024 ground with the real pmove chain. The ground is a
// Bullet heightfield, not brushes, so the traces take the convex-sweep
// path: this checks that path stands the player on the terrain, keeps
// them there on the flat, and carries them up a hillside.

#include "q3_test_walk.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_collision.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace impl = sdl3cpp::services::impl;
using q3test::Scene;
using q3test::WalkForward;

namespace {

/// 81 m square at 1 m, flat to x = 0 and rising at `degrees` beyond.
impl::Fs2024Heightfield Hillside(float degrees) {
    impl::Fs2024Heightfield field;
    field.columns = 81;
    field.rows = 81;
    field.spacing = 1.f;
    field.origin = {-40.f, -40.f};
    const float rise = std::tan(degrees * 3.14159265f / 180.f);
    for (int r = 0; r < field.rows; ++r) {
        for (int c = 0; c < field.columns; ++c) {
            const float x = field.origin.x + static_cast<float>(c);
            field.heights.push_back(std::max(x, 0.f) * rise);
        }
    }
    field.minHeight = 0.f;
    field.maxHeight = field.heights.back();
    return field;
}

/// A scene whose only geometry is the heightfield: no brushes, so pmove
/// sweeps Bullet shapes as it does over FS2024 ground.
struct TerrainScene {
    Scene scene;
    impl::Fs2024Heightfield field;
    impl::Fs2024TerrainCollision collision;

    explicit TerrainScene(float degrees) : field(Hillside(degrees)) {
        scene.world.setWorldUserInfo(nullptr);
        collision = impl::AddFs2024TerrainCollision(&scene.world, field);
    }
    ~TerrainScene() {
        impl::RemoveFs2024TerrainCollision(&scene.world, collision);
    }
};

}  // namespace

TEST(Fs2024TerrainWalk, FlatGroundHoldsThePlayer) {
    TerrainScene terrain(0.f);
    const auto result = WalkForward(terrain.scene, true);
    EXPECT_GT(result.travelledX, 3.f);
    EXPECT_NEAR(result.rise, 0.f, 0.05f);
}

TEST(Fs2024TerrainWalk, WalksUpAHillside) {
    TerrainScene terrain(15.f);
    const auto result = WalkForward(terrain.scene, true);
    EXPECT_GT(result.travelledX, 4.f);
    // Wherever they got to, they stand on the slope there: a flat-bottomed
    // box rests on its uphill edge, up to 0.47 * tan 15 above the centre.
    const float expected =
        std::max(result.travelledX - 3.f, 0.f) * std::tan(0.2618f);
    EXPECT_NEAR(result.rise, expected, 0.2f);
}

TEST(Fs2024TerrainWalk, SlowWalkUpAHillsideDoesNotStick) {
    TerrainScene terrain(20.f);
    const auto result = WalkForward(terrain.scene, true, 0.3f);
    EXPECT_GT(result.travelledX, 1.5f);
    EXPECT_GT(result.rise, 0.f);
}
