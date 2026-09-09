// Frame timestep clamping. Steps that read a fixed default instead of
// real elapsed time move the player per frame rather than per second,
// so the faster the machine the faster the player travels.

#include "services/interfaces/workflow/quake3/q3_frame_time.hpp"

#include <gtest/gtest.h>
#include <cmath>

namespace q3 = sdl3cpp::q3;

TEST(ClampFrameSeconds, PassesATypicalFrameThrough) {
    EXPECT_FLOAT_EQ(q3::ClampFrameSeconds(0.0085f), 0.0085f);
    EXPECT_FLOAT_EQ(q3::ClampFrameSeconds(0.016f), 0.016f);
}

TEST(ClampFrameSeconds, CapsALongStallAtIoq3sSixtySixMilliseconds) {
    EXPECT_FLOAT_EQ(q3::ClampFrameSeconds(5.0f), q3::kMaxFrameSeconds);
    EXPECT_FLOAT_EQ(q3::kMaxFrameSeconds, 0.066f);
}

TEST(ClampFrameSeconds, RaisesAStalledFrameToTheFloor) {
    EXPECT_FLOAT_EQ(q3::ClampFrameSeconds(0.0f), q3::kMinFrameSeconds);
}

TEST(ClampFrameSeconds, RejectsNegativeTime) {
    EXPECT_FLOAT_EQ(q3::ClampFrameSeconds(-1.0f), q3::kMinFrameSeconds);
}

TEST(ClampFrameSeconds, RejectsNaN) {
    EXPECT_FLOAT_EQ(q3::ClampFrameSeconds(std::nanf("")),
                    q3::kMinFrameSeconds);
}

TEST(ClampFrameSeconds, IsMonotonic) {
    float previous = 0.0f;
    for (float s = 0.0f; s < 0.2f; s += 0.005f) {
        const float clamped = q3::ClampFrameSeconds(s);
        EXPECT_GE(clamped, previous);
        previous = clamped;
    }
}

TEST(ClampFrameSeconds, DistanceScalesWithTimeNotFrameCount) {
    // The regression: at 117fps a fixed 0.016 default travels 1.88x
    // further per second than the real 0.0085 elapsed does.
    constexpr float speed = 10.0f;
    const float realStep = q3::ClampFrameSeconds(0.0085f) * speed;
    const float fixedStep = 0.016f * speed;
    EXPECT_NEAR(fixedStep / realStep, 1.88f, 0.01f);
}
