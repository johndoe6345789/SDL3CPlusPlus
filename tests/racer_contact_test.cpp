#include "services/interfaces/workflow/racer/player/racer_field_rules.hpp"
#include "services/interfaces/workflow/racer/player/racer_line_guide.hpp"

#include <gtest/gtest.h>

#include <cmath>

using namespace sdl3cpp::services::impl;

TEST(RacerField, TheHeavierPodGivesWayLess) {
    RacerPodState light;
    RacerPodState heavy;
    heavy.mass = 80.f;   // Sebulba's bump mass against Anakin's 50
    heavy.position = {1.f, 0.f, 0.f};
    SeparateRacerPods({&light, &heavy});
    EXPECT_GT(-light.position.x, heavy.position.x - 1.f);
}

TEST(RacerField, AiPodsAreDrawnBackToTheLine) {
    using sdl3cpp::services::impl::GuideRacerPodToLine;
    // A straight line along -z; the pod has strayed 10 m to the side.
    std::vector<glm::vec3> line;
    for (int i = 0; i < 20; ++i) line.push_back({0.f, 0.f, -4.f * i});
    RacerPodState pod;
    pod.grounded = true;
    pod.position = {10.f, 0.f, -8.f};
    for (int frame = 0; frame < 120; ++frame) {
        GuideRacerPodToLine(pod, line, 2, 3.5f, 20.f, 1.f / 60.f);
    }
    EXPECT_NEAR(std::fabs(pod.position.x), 3.5f, 0.01f);
    EXPECT_FLOAT_EQ(pod.position.z, -8.f);  // never moved along the line
}
