// Roof shapes over a real footprint's oriented box: a terrace's ridge
// must run along its LONG axis (a gable turned 90 degrees is the
// give-away of a roof built over an axis-aligned box instead), hips
// must pull their ridge in, and every roof triangle must face up.

#include "services/interfaces/workflow/fs2024/building/fs2024_roof_mesh.hpp"

#include "services/interfaces/workflow/fs2024/building/fs2024_oriented_box.hpp"

#include <gtest/gtest.h>

#include <cmath>

namespace impl = sdl3cpp::services::impl;

namespace {

/// A 20 m by 6 m terrace row, turned 30 degrees off the axes.
std::vector<impl::Point2> Terrace() {
    const float angle = 30.f * 3.14159265f / 180.f;
    const float c = std::cos(angle), s = std::sin(angle);
    std::vector<impl::Point2> out;
    for (auto [x, y] : {std::pair{-10.f, -3.f}, {10.f, -3.f},
                        {10.f, 3.f}, {-10.f, 3.f}}) {
        out.push_back({x * c - y * s, x * s + y * c});
    }
    return out;
}

float RidgeHeight(const std::vector<impl::BspRenderVertex>& vertices) {
    float top = 0.f;
    for (const auto& vertex : vertices) top = std::max(top, vertex.y);
    return top;
}

}  // namespace

TEST(Fs2024RoofMesh, OrientedBoxFindsTheTerracesLongAxis) {
    const impl::OrientedBox box = impl::ComputeOrientedBox(Terrace());
    EXPECT_NEAR(box.halfLength, 10.f, 0.01f);
    EXPECT_NEAR(box.halfWidth, 3.f, 0.01f);
    EXPECT_NEAR(std::abs(box.longAxis.x), std::cos(30.f * 3.14159265f / 180.f),
               0.01f);
}

TEST(Fs2024RoofMesh, GabledRidgeRunsTheWholeLengthAndFacesUp) {
    std::vector<impl::BspRenderVertex> vertices;
    std::vector<std::uint32_t> indices;
    const float rise = impl::AppendRoofMesh(Terrace(), 7.f,
                                           impl::RoofShape::Gabled, 2.5f, 4.f,
                                           vertices, indices);
    EXPECT_FLOAT_EQ(rise, 2.5f);
    EXPECT_FLOAT_EQ(RidgeHeight(vertices), 9.5f);
    // Slopes face up; a gable end is vertical and faces outwards, so
    // it has no upward normal at all -- but nothing may face down.
    const impl::OrientedBox shape = impl::ComputeOrientedBox(Terrace());
    for (const auto& vertex : vertices) {
        EXPECT_GE(vertex.ny, 0.f);
        if (vertex.ny > 1e-4f) continue;
        const float outX = vertex.x - shape.centre.x;
        const float outZ = vertex.z - shape.centre.y;
        EXPECT_GT(vertex.nx * outX + vertex.nz * outZ, 0.f);
    }

    int atRidge = 0;
    for (const auto& vertex : vertices) {
        if (std::abs(vertex.y - 9.5f) < 1e-4f) ++atRidge;
    }
    EXPECT_GT(atRidge, 0);
    // The two ridge ends are 20 m apart: the terrace's own length.
    float minAlong = 1e30f, maxAlong = -1e30f;
    const impl::OrientedBox box = impl::ComputeOrientedBox(Terrace());
    for (const auto& vertex : vertices) {
        if (std::abs(vertex.y - 9.5f) > 1e-4f) continue;
        const float along = vertex.x * box.longAxis.x +
                           vertex.z * box.longAxis.y;
        minAlong = std::min(minAlong, along);
        maxAlong = std::max(maxAlong, along);
    }
    EXPECT_NEAR(maxAlong - minAlong, 20.f, 0.05f);
}

TEST(Fs2024RoofMesh, HippedRidgePullsInByTheRoofsHalfWidth) {
    std::vector<impl::BspRenderVertex> vertices;
    std::vector<std::uint32_t> indices;
    impl::AppendRoofMesh(Terrace(), 7.f, impl::RoofShape::Hipped, 2.f, 4.f,
                        vertices, indices);
    const impl::OrientedBox box = impl::ComputeOrientedBox(Terrace());
    float minAlong = 1e30f, maxAlong = -1e30f;
    for (const auto& vertex : vertices) {
        if (std::abs(vertex.y - 9.f) > 1e-4f) continue;
        const float along = vertex.x * box.longAxis.x +
                           vertex.z * box.longAxis.y;
        minAlong = std::min(minAlong, along);
        maxAlong = std::max(maxAlong, along);
    }
    EXPECT_NEAR(maxAlong - minAlong, 14.f, 0.05f);  // 20 - 2 * 3
}

TEST(Fs2024RoofMesh, FlatRoofIsTheFootprintItself) {
    std::vector<impl::BspRenderVertex> vertices;
    std::vector<std::uint32_t> indices;
    const float rise = impl::AppendRoofMesh(Terrace(), 12.f,
                                           impl::RoofShape::Flat, 3.f, 8.f,
                                           vertices, indices);
    EXPECT_FLOAT_EQ(rise, 0.f);
    EXPECT_EQ(vertices.size(), 4u);
    EXPECT_EQ(indices.size(), 6u);
    for (const auto& vertex : vertices) EXPECT_FLOAT_EQ(vertex.y, 12.f);
}
