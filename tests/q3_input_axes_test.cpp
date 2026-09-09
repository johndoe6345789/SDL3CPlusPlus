// Feeds a synthetic keyboard state through the real input.axis.combine
// step using the shipped quake3 binding config, and checks each key
// produces the axis the movement chain expects. The pmove chain is
// provably symmetric, so a direction that feels reluctant has to be
// losing its input before it gets there.

#include "services/interfaces/workflow/input/workflow_input_axis_combine_step.hpp"
#include "services/interfaces/workflow/input/workflow_input_button_combine_step.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <string>

namespace impl = sdl3cpp::services::impl;

namespace {

constexpr const char* kConfig =
    "packages/quake3/config/input_aggregation.json";

// input.keyboard.poll publishes only the keys currently held, named by
// SDL_GetScancodeName.
float AxisFor(const std::string& heldKey, const char* axisKey) {
    sdl3cpp::services::WorkflowContext context;
    nlohmann::json keys = nlohmann::json::object();
    if (!heldKey.empty()) keys[heldKey] = true;
    context.Set<nlohmann::json>("input.keyboard.state", keys);

    sdl3cpp::services::WorkflowStepDefinition step;
    step.parameters["config_path"].type =
        sdl3cpp::services::WorkflowParameterValue::Type::String;
    step.parameters["config_path"].stringValue = kConfig;

    impl::WorkflowInputAxisCombineStep combine(nullptr);
    combine.Execute(step, context);
    return context.Get<float>(axisKey, 0.0f);
}

bool ButtonFor(const std::string& heldKey, const char* buttonKey) {
    sdl3cpp::services::WorkflowContext context;
    nlohmann::json keys = nlohmann::json::object();
    if (!heldKey.empty()) keys[heldKey] = true;
    context.Set<nlohmann::json>("input.keyboard.state", keys);

    sdl3cpp::services::WorkflowStepDefinition step;
    step.parameters["config_path"].type =
        sdl3cpp::services::WorkflowParameterValue::Type::String;
    step.parameters["config_path"].stringValue = kConfig;

    impl::WorkflowInputButtonCombineStep combine(nullptr);
    combine.Execute(step, context);
    return context.GetBool(buttonKey, false);
}

}  // namespace

TEST(InputAxes, WDrivesForward) {
    EXPECT_FLOAT_EQ(AxisFor("W", "input.move_forward"), 1.0f);
}

TEST(InputAxes, SDrivesBackward) {
    EXPECT_FLOAT_EQ(AxisFor("S", "input.move_forward"), -1.0f);
}

TEST(InputAxes, DDrivesRight) {
    EXPECT_FLOAT_EQ(AxisFor("D", "input.move_right"), 1.0f);
}

TEST(InputAxes, ADrivesLeft) {
    EXPECT_FLOAT_EQ(AxisFor("A", "input.move_right"), -1.0f);
}

TEST(InputAxes, EveryDirectionHasTheSameMagnitude) {
    // A direction that produces a smaller magnitude is the one that
    // feels reluctant in game.
    EXPECT_FLOAT_EQ(std::abs(AxisFor("W", "input.move_forward")),
                    std::abs(AxisFor("S", "input.move_forward")));
    EXPECT_FLOAT_EQ(std::abs(AxisFor("D", "input.move_right")),
                    std::abs(AxisFor("A", "input.move_right")));
    EXPECT_FLOAT_EQ(std::abs(AxisFor("W", "input.move_forward")),
                    std::abs(AxisFor("D", "input.move_right")));
}

TEST(InputAxes, NothingHeldMeansNoMovement) {
    EXPECT_FLOAT_EQ(AxisFor("", "input.move_forward"), 0.0f);
    EXPECT_FLOAT_EQ(AxisFor("", "input.move_right"), 0.0f);
}

TEST(InputAxes, OneAxisDoesNotLeakIntoTheOther) {
    EXPECT_FLOAT_EQ(AxisFor("W", "input.move_right"), 0.0f);
    EXPECT_FLOAT_EQ(AxisFor("D", "input.move_forward"), 0.0f);
}

TEST(InputButtons, SpaceJumpsAndCtrlCrouches) {
    EXPECT_TRUE(ButtonFor("Space", "input.jump"));
    EXPECT_TRUE(ButtonFor("Left Ctrl", "input.crouch"));
    EXPECT_FALSE(ButtonFor("", "input.jump"));
}
