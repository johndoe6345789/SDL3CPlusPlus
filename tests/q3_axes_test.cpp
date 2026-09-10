// The axis convention, pinned. Every draw site used to build its own
// model basis by hand, and all of them got the same thing wrong: a
// decoded MD3's local +Y is up, but the hand-rolled matrices put world
// "right" on column 1, so every model was laid on its side.

#include "services/interfaces/workflow/quake3/q3_axes.hpp"

#include <gtest/gtest.h>

namespace {

constexpr float kPi = 3.14159265358979f;

// Quake model (0, 0, 10) — above the head — after the loader's decode.
glm::vec3 DecodedHead(sdl3cpp::q3::AxisConvention c) {
    return sdl3cpp::q3::FromQuakeDir(glm::vec3(0.0f, 0.0f, 10.0f), c);
}

// Quake model (10, 0, 0) — out the front.
glm::vec3 DecodedNose(sdl3cpp::q3::AxisConvention c) {
    return sdl3cpp::q3::FromQuakeDir(glm::vec3(10.0f, 0.0f, 0.0f), c);
}

void ExpectVec(const glm::vec3& got, const glm::vec3& want) {
    EXPECT_NEAR(got.x, want.x, 1e-4f);
    EXPECT_NEAR(got.y, want.y, 1e-4f);
    EXPECT_NEAR(got.z, want.z, 1e-4f);
}

}  // namespace

using sdl3cpp::q3::AxisConvention;

class Axes : public ::testing::TestWithParam<AxisConvention> {};

TEST_P(Axes, AModelStandsUpAtEveryYaw) {
    const AxisConvention c = GetParam();
    const glm::vec3 up     = sdl3cpp::q3::AxisUp(c);
    for (float deg : {0.0f, 45.0f, 90.0f, 180.0f, 270.0f}) {
        const glm::mat4 m =
            sdl3cpp::q3::PlaceModel(glm::vec3(0.0f), deg * kPi / 180.0f, c);
        const glm::vec4 head(DecodedHead(c), 1.0f);
        ExpectVec(glm::vec3(m * head), up * 10.0f);
    }
}

TEST_P(Axes, AModelFacesItsYaw) {
    const AxisConvention c = GetParam();
    for (float deg : {0.0f, 90.0f, 180.0f}) {
        const float yaw   = deg * kPi / 180.0f;
        const glm::mat4 m = sdl3cpp::q3::PlaceModel(glm::vec3(0.0f), yaw, c);
        const glm::vec4 nose(DecodedNose(c), 1.0f);
        ExpectVec(glm::vec3(m * nose), sdl3cpp::q3::YawForward(yaw, c) * 10.0f);
    }
}

TEST_P(Axes, YawTowardsInvertsYawForward) {
    const AxisConvention c = GetParam();
    for (float deg : {0.0f, 30.0f, 120.0f, -95.0f}) {
        const float yaw = deg * kPi / 180.0f;
        EXPECT_NEAR(
            sdl3cpp::q3::YawTowards(sdl3cpp::q3::YawForward(yaw, c) * 7.0f, c),
            yaw, 1e-4f);
    }
}

TEST_P(Axes, TheModelBasisIsARotation) {
    // Determinant +1, or the model is mirrored rather than turned.
    const AxisConvention c = GetParam();
    const glm::mat4 b      = sdl3cpp::q3::ModelBasis(
        sdl3cpp::q3::YawForward(0.7f, c), sdl3cpp::q3::AxisUp(c), c);
    EXPECT_NEAR(glm::determinant(glm::mat3(b)), 1.0f, 1e-4f);
}

TEST(AxesConvention, TheEngineCoreDefaultsToOneConvention) {
    const AxisConvention was = sdl3cpp::q3::ActiveAxisConvention();
    sdl3cpp::q3::SetActiveAxisConvention(AxisConvention::Ioq3ZUp);
    ExpectVec(sdl3cpp::q3::AxisUp(), glm::vec3(0.0f, 0.0f, 1.0f));
    sdl3cpp::q3::SetActiveAxisConvention(AxisConvention::EngineYUp);
    ExpectVec(sdl3cpp::q3::AxisUp(), glm::vec3(0.0f, 1.0f, 0.0f));
    sdl3cpp::q3::SetActiveAxisConvention(was);
}

INSTANTIATE_TEST_SUITE_P(BothConventions, Axes,
                         ::testing::Values(AxisConvention::Ioq3ZUp,
                                           AxisConvention::EngineYUp));
