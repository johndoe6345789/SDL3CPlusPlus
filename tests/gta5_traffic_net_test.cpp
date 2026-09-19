#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_ai.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

using sdl3cpp::services::impl::Gta5BaselineDriver;
using sdl3cpp::services::impl::Gta5DriverInputs;
using sdl3cpp::services::impl::Gta5DriverNet;
using sdl3cpp::services::impl::Gta5DriverSense;

/// A single 8 -> 3 layer in the trainer's TNN1 layout.
std::string WriteNet(const std::vector<float>& weights,
                     const std::vector<float>& bias) {
    const std::string path = "traffic_net_test.tnn";
    std::FILE* file = std::fopen(path.c_str(), "wb");
    const std::uint32_t head[2] = {0x314E4E54u, 1u};
    const std::int32_t shape[2] = {8, 3};
    std::fwrite(head, sizeof(head), 1, file);
    std::fwrite(shape, sizeof(shape), 1, file);
    std::fwrite(weights.data(), sizeof(float), weights.size(), file);
    std::fwrite(bias.data(), sizeof(float), bias.size(), file);
    std::fclose(file);
    return path;
}

TEST(Gta5TrafficNetTest, ClearRoadAccelerates) {
    Gta5DriverSense sense;
    sense.speed = 2.f;
    const auto act = Gta5BaselineDriver(sense);
    EXPECT_GT(act.throttle, 0.5f);
    EXPECT_FLOAT_EQ(act.brake, 0.f);
}

TEST(Gta5TrafficNetTest, RedLightAheadStops) {
    Gta5DriverSense sense;
    sense.speed = 9.f;
    sense.line = 10.f;
    const auto act = Gta5BaselineDriver(sense);
    EXPECT_FLOAT_EQ(act.throttle, 0.f);
    EXPECT_GT(act.brake, 0.f);
}

TEST(Gta5TrafficNetTest, NetMatchesHandComputedLayer) {
    std::vector<float> w(24, 0.f), b = {0.f, 0.5f, -0.5f};
    w[0 * 8 + 1] = 2.f;  // steer reads the scaled `off`
    w[1 * 8 + 0] = 1.f;  // throttle reads the scaled speed
    Gta5DriverNet net;
    ASSERT_TRUE(net.Load(WriteNet(w, b)));
    Gta5DriverSense sense;
    sense.speed = 10.f;
    sense.off = 0.5f;
    float in[8];
    Gta5DriverInputs(sense, in);
    const auto act = net.Run(sense);
    EXPECT_NEAR(act.steer, std::tanh(2.f * in[1]), 1e-6f);
    EXPECT_NEAR(act.throttle, 1.f / (1.f + std::exp(-(in[0] + 0.5f))),
                1e-6f);
    EXPECT_NEAR(act.brake, 1.f / (1.f + std::exp(0.5f)), 1e-6f);
    std::remove("traffic_net_test.tnn");
}

TEST(Gta5TrafficNetTest, RejectsWrongShape) {
    Gta5DriverNet net;
    EXPECT_FALSE(net.Load("no_such_file.tnn"));
    EXPECT_FALSE(net.Ready());
}

}  // namespace
