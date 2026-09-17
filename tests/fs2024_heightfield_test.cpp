// The FS2024 ground: reading terrain.fst, the height under a point, the
// drawn mesh, and the guard that keeps the player on the field. The
// drawn triangles, Fs2024HeightAt and Bullet's heightfield must agree
// on which way each cell is split, or the player floats or sinks.

#include "services/interfaces/workflow/fs2024/terrain/fs2024_heightfield.hpp"
#include "services/interfaces/workflow/fs2024/player/fs2024_player_place.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_collision.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_mesh.hpp"
#include "services/interfaces/workflow/quake3/pmove/q3_wish_dir.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace impl = sdl3cpp::services::impl;

namespace {

/// 3x3 grid, 10 m apart, origin (-10, -10): a saddle, so the two ways
/// of splitting a cell give different heights.
impl::Fs2024Heightfield Saddle() {
    impl::Fs2024Heightfield field;
    field.columns = 3;
    field.rows = 3;
    field.spacing = 10.f;
    field.origin = {-10.f, -10.f};
    field.heights = {0.f, 4.f, 0.f,  //
                     4.f, 0.f, 4.f,  //
                     0.f, 4.f, 0.f};
    field.minHeight = 0.f;
    field.maxHeight = 4.f;
    return field;
}

std::filesystem::path WriteFile(const std::string& name,
                                const std::string& magic) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream out(path, std::ios::binary);
    out.write(magic.data(), 4);
    const std::uint32_t dims[2] = {2, 2};
    out.write(reinterpret_cast<const char*>(dims), sizeof(dims));
    const float header[3] = {5.f, -5.f, -5.f};
    out.write(reinterpret_cast<const char*>(header), sizeof(header));
    const float heights[4] = {1.f, 2.f, 3.f, -4.f};
    out.write(reinterpret_cast<const char*>(heights), sizeof(heights));
    return path;
}

float BulletHeight(btDiscreteDynamicsWorld& world, float x, float z) {
    const btVector3 from(x, 100.f, z), to(x, -100.f, z);
    btCollisionWorld::ClosestRayResultCallback hit(from, to);
    world.rayTest(from, to, hit);
    EXPECT_TRUE(hit.hasHit()) << "at " << x << ", " << z;
    return hit.m_hitPointWorld.y();
}

}  // namespace

TEST(Fs2024Heightfield, ReadsWhatTheBakeWrites) {
    const auto path = WriteFile("fs2024_ok.fst", "FST1");
    const impl::Fs2024Heightfield field =
        impl::ReadFs2024Heightfield(path.string());
    EXPECT_EQ(field.columns, 2);
    EXPECT_EQ(field.rows, 2);
    EXPECT_FLOAT_EQ(field.spacing, 5.f);
    EXPECT_FLOAT_EQ(field.At(0, 1), 3.f);
    EXPECT_FLOAT_EQ(field.minHeight, -4.f);
    EXPECT_FLOAT_EQ(field.maxHeight, 3.f);
    EXPECT_EQ(field.Position(1, 1), glm::vec3(0.f, -4.f, 0.f));
}

TEST(Fs2024Heightfield, RejectsOtherFiles) {
    const auto path = WriteFile("fs2024_bad.fst", "NOPE");
    EXPECT_THROW(impl::ReadFs2024Heightfield(path.string()),
                 std::runtime_error);
    EXPECT_THROW(impl::ReadFs2024Heightfield("no/such/file.fst"),
                 std::runtime_error);
}

TEST(Fs2024Heightfield, HeightAtMatchesBulletsSplit) {
    const impl::Fs2024Heightfield field = Saddle();
    btDefaultCollisionConfiguration config;
    btCollisionDispatcher dispatcher(&config);
    btDbvtBroadphase broadphase;
    btSequentialImpulseConstraintSolver solver;
    btDiscreteDynamicsWorld world(&dispatcher, &broadphase, &solver,
                                  &config);
    impl::Fs2024TerrainCollision collision =
        impl::AddFs2024TerrainCollision(&world, field);

    // Points either side of both possible diagonals in every cell.
    for (float x : {-8.f, -2.f, 2.f, 8.f}) {
        for (float z : {-7.f, -3.f, 3.f, 7.f}) {
            EXPECT_NEAR(impl::Fs2024HeightAt(field, x, z),
                        BulletHeight(world, x, z), 0.01f)
                << "at " << x << ", " << z;
        }
    }
    EXPECT_FLOAT_EQ(impl::Fs2024HeightAt(field, 0.f, 0.f), 0.f);
    EXPECT_FLOAT_EQ(impl::Fs2024HeightAt(field, -50.f, 0.f), 4.f);
    impl::RemoveFs2024TerrainCollision(&world, collision);
}

TEST(Fs2024TerrainMesh, TrianglesFaceUpAndMatchHeightAt) {
    const impl::Fs2024Heightfield field = Saddle();
    const impl::Fs2024TerrainChunkMesh mesh =
        impl::BuildFs2024TerrainChunk(field, 0, 0, 64);
    ASSERT_EQ(mesh.vertices.size(), 9u);
    ASSERT_EQ(mesh.indices.size(), 24u);
    EXPECT_FLOAT_EQ(mesh.max.y, 4.f);
    EXPECT_FLOAT_EQ(mesh.vertices[8].u, 1.f);

    for (std::size_t i = 0; i < mesh.indices.size(); i += 3) {
        const auto& a = mesh.vertices[mesh.indices[i]];
        const auto& b = mesh.vertices[mesh.indices[i + 1]];
        const auto& c = mesh.vertices[mesh.indices[i + 2]];
        const glm::vec3 pa(a.x, a.y, a.z), pb(b.x, b.y, b.z),
                        pc(c.x, c.y, c.z);
        // Counter-clockwise from above is an upward normal.
        EXPECT_GT(glm::cross(pb - pa, pc - pa).y, 0.f);
        // Each triangle's centroid is on the surface HeightAt describes.
        const glm::vec3 centre = (pa + pb + pc) / 3.f;
        EXPECT_NEAR(impl::Fs2024HeightAt(field, centre.x, centre.z),
                    centre.y, 1e-4f);
    }
}

TEST(Fs2024Player, HeadingBecomesTheYawPmoveWalksAlong) {
    for (float heading : {0.f, 90.f, 180.f, 261.f}) {
        const auto wish = sdl3cpp::q3::ComputeWish(
            1.f, 0.f, impl::Fs2024YawForHeading(heading), 1.f);
        const float h = heading * 3.14159265f / 180.f;
        EXPECT_NEAR(wish.direction.x, std::sin(h), 1e-4f) << heading;
        EXPECT_NEAR(wish.direction.z, -std::cos(h), 1e-4f) << heading;
    }
}

TEST(Fs2024Player, GuardReturnsASunkPlayerButLeavesAStandingOneAlone) {
    const impl::Fs2024Heightfield field = Saddle();
    impl::Q3PlayerState player;
    player.origin = {0.f, -5.f, 0.f};
    player.velocity = {1.f, -20.f, 1.f};
    ASSERT_TRUE(impl::Fs2024KeepOnGround(field, player));
    EXPECT_NEAR(player.origin.y + player.mins.y, 0.05f, 1e-4f);
    EXPECT_FLOAT_EQ(player.velocity.y, 0.f);

    // Standing properly on the ground is left alone.
    EXPECT_FALSE(impl::Fs2024KeepOnGround(field, player));
}

// Finding *which* tile's field to pass in here is the tile map's job,
// not this one's -- see fs2024_tile_streaming_test.cpp's
// Fs2024TileLookup tests.
