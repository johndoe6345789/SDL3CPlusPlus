#include "services/interfaces/workflow/stunts/data/stunts_shape.hpp"

#include <gtest/gtest.h>

using sdl3cpp::services::impl::ParseStuntsShape;
using sdl3cpp::services::impl::StuntsPrimitiveKind;

namespace {

/// Builds a minimal shape: 4 vertices, one quad, 2 paint jobs.
std::vector<std::uint8_t> QuadShape() {
    std::vector<std::uint8_t> data = {4, 1, 2, 0};  // nV nP nPaint reserved
    // Four vertices, a flat square at y=0.
    const std::int16_t coords[4][3] = {
        {0, 0, 0}, {10, 0, 0}, {10, 0, 10}, {0, 0, 10}};
    for (const auto& v : coords) {
        for (const std::int16_t c : v) {
            data.push_back(static_cast<std::uint8_t>(c));
            data.push_back(static_cast<std::uint8_t>(c >> 8));
        }
    }
    data.insert(data.end(), 8, 0);  // one primitive's culling block
    // type=4 (quad), flags=0, materials[2]={5,6}, indices={0,1,2,3}.
    const std::vector<std::uint8_t> prim = {4, 0, 5, 6, 0, 1, 2, 3};
    data.insert(data.end(), prim.begin(), prim.end());
    return data;
}

/// A wheel primitive: type 12, fixed six-byte index payload.
std::vector<std::uint8_t> WheelShape() {
    std::vector<std::uint8_t> data = {3, 1, 1, 0};
    const std::int16_t coords[3][3] = {{0, 0, 0}, {0, 5, 0}, {0, 0, 5}};
    for (const auto& v : coords) {
        for (const std::int16_t c : v) {
            data.push_back(static_cast<std::uint8_t>(c));
            data.push_back(static_cast<std::uint8_t>(c >> 8));
        }
    }
    data.insert(data.end(), 8, 0);
    // type=12, flags=0, material[1]={9}, payload={0,1,2,0,1,2}.
    const std::vector<std::uint8_t> prim = {12, 0, 9, 0, 1, 2, 0, 1, 2};
    data.insert(data.end(), prim.begin(), prim.end());
    return data;
}

}  // namespace

TEST(StuntsShape, ParsesVerticesAndOneQuadFace) {
    const auto shape = ParseStuntsShape(QuadShape());
    ASSERT_TRUE(shape.valid);
    ASSERT_EQ(shape.vertices.size(), 4u);
    EXPECT_EQ(shape.vertices[1].x, 10);
    ASSERT_EQ(shape.faces.size(), 1u);
    const auto& face = shape.faces[0];
    EXPECT_EQ(face.kind, StuntsPrimitiveKind::Polygon);
    EXPECT_EQ(face.materials, (std::vector<std::uint8_t>{5, 6}));
    EXPECT_EQ(face.indices, (std::vector<std::uint8_t>{0, 1, 2, 3}));
}

TEST(StuntsShape, ParsesAWheelPrimitive) {
    const auto shape = ParseStuntsShape(WheelShape());
    ASSERT_TRUE(shape.valid);
    ASSERT_EQ(shape.faces.size(), 1u);
    const auto& face = shape.faces[0];
    EXPECT_EQ(face.kind, StuntsPrimitiveKind::Wheel);
    EXPECT_EQ(face.materials, (std::vector<std::uint8_t>{9}));
    EXPECT_EQ(face.payload,
             (std::array<std::uint8_t, 6>{0, 1, 2, 0, 1, 2}));
}

TEST(StuntsShape, RejectsAnOutOfRangeVertexIndex) {
    auto data = QuadShape();
    data.back() = 99;  // last index byte, now well past nV=4
    EXPECT_FALSE(ParseStuntsShape(data).valid);
}

TEST(StuntsShape, RejectsATruncatedBuffer) {
    auto data = QuadShape();
    data.resize(data.size() - 3);
    EXPECT_FALSE(ParseStuntsShape(data).valid);
}

TEST(StuntsShape, RejectsAnEmptyBuffer) {
    EXPECT_FALSE(ParseStuntsShape({}).valid);
}
