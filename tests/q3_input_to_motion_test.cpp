// The whole chain a key press takes: keyboard state -> axis.combine ->
// pmove -> velocity. Both halves pass on their own, so this joins them:
// a bug in the handover between them would hide from either test.

#include "services/interfaces/workflow/input/workflow_input_axis_combine_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_accelerate_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <string>

namespace impl = sdl3cpp::services::impl;
using sdl3cpp::services::WorkflowContext;
using sdl3cpp::services::WorkflowParameterValue;
using sdl3cpp::services::WorkflowStepDefinition;

namespace {

// Press one key and report the velocity a single accelerate tick gives.
glm::vec3 VelocityFromKey(const std::string& key, float yaw) {
    WorkflowContext context;
    nlohmann::json keys = nlohmann::json::object();
    keys[key] = true;
    context.Set<nlohmann::json>("input.keyboard.state", keys);
    context.Set<float>("q3.player_yaw", yaw);
    context.Set<double>("frame.delta_time", 1.0 / 125.0);

    impl::Q3PlayerState ps;
    ps.onGround = true;
    context.Set("q3.ps", ps);

    WorkflowStepDefinition combineStep;
    auto& path = combineStep.parameters["config_path"];
    path.type = WorkflowParameterValue::Type::String;
    path.stringValue = "packages/quake3/config/input_aggregation.json";

    impl::WorkflowInputAxisCombineStep(nullptr).Execute(combineStep, context);
    impl::WorkflowQ3PmAccelerateStep(nullptr).Execute(WorkflowStepDefinition{},
                                                      context);
    return context.Get<impl::Q3PlayerState>("q3.ps",
                                            impl::Q3PlayerState{}).velocity;
}

}  // namespace

TEST(InputToMotion, ForwardMovesTheExpectedWay) {
    const auto v = VelocityFromKey("W", 0.0f);
    EXPECT_LT(v.z, -0.001f) << "W at yaw 0 should drive -Z";
}

TEST(InputToMotion, BackMovesTheOppositeWay) {
    const auto v = VelocityFromKey("S", 0.0f);
    EXPECT_GT(v.z, 0.001f);
}

TEST(InputToMotion, StrafeRightMoves) {
    const auto v = VelocityFromKey("D", 0.0f);
    EXPECT_GT(v.x, 0.001f) << "D at yaw 0 should drive +X";
}

TEST(InputToMotion, StrafeLeftMoves) {
    const auto v = VelocityFromKey("A", 0.0f);
    EXPECT_LT(v.x, -0.001f) << "A at yaw 0 should drive -X";
}

TEST(InputToMotion, StrafeAccelerationMatchesForward) {
    const float forward = std::abs(VelocityFromKey("W", 0.0f).z);
    const float strafe = std::abs(VelocityFromKey("D", 0.0f).x);
    ASSERT_GT(forward, 0.001f);
    EXPECT_NEAR(strafe, forward, forward * 0.01f)
        << "strafing should accelerate as hard as walking forward";
}

TEST(InputToMotion, EveryKeyProducesMotion) {
    for (const char* key : {"W", "A", "S", "D"}) {
        const auto v = VelocityFromKey(key, 0.7f);
        EXPECT_GT(glm::length(v), 0.001f) << "key " << key << " did nothing";
    }
}
