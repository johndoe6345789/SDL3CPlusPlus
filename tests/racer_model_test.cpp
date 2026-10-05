#include "racer_model_fixture.hpp"

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;
using racer_test::OneTriangleModel;


TEST(RacerModel, ReplaysDisplayListIntoOneTriangle) {
    const RacerModel model = ParseRacerModel(OneTriangleModel());
    ASSERT_TRUE(model.valid);
    EXPECT_EQ(model.tag, 0x5472616Bu);
    EXPECT_EQ(model.triangleCount, 1);
    ASSERT_EQ(model.batches.size(), 1u);
    ASSERT_EQ(model.batches[0].vertices.size(), 3u);
}

TEST(RacerModel, AppliesNodeTransformAndUvScale) {
    const RacerModel model = ParseRacerModel(OneTriangleModel());
    const auto& v = model.batches[0].vertices;
    EXPECT_FLOAT_EQ(v[1].x, 110.f);  // 10 + translation 100
    EXPECT_FLOAT_EQ(v[1].u, 1.f);    // 4096 units span the image
    EXPECT_EQ(v[1].r, 0xFF);
    EXPECT_EQ(v[1].g, 0x80);
}

TEST(RacerModel, ReadsMaterialTexture) {
    const RacerModel model = ParseRacerModel(OneTriangleModel());
    const auto& m = model.batches[0].material;
    EXPECT_EQ(m.textureIndex, 7);
    EXPECT_EQ(m.format, RacerTextureFormat::Indexed4);
    EXPECT_EQ(m.width, 32);
    EXPECT_EQ(m.height, 16);
}

TEST(RacerModel, RejectsTruncatedData) {
    EXPECT_FALSE(ParseRacerModel({0x54, 0x72}).valid);
}
