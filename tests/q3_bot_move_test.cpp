// Bots move by producing a usercmd and letting pmove resolve it, the
// way ioq3 does (g_active.c G_RunClient -> ClientThink_real -> Pmove).
// Before that they added a velocity straight onto their origin, so they
// hovered at spawn height and walked through walls.

#include "services/interfaces/workflow/quake3/q3_axes.hpp"
#include "services/interfaces/workflow/quake3/q3_bot_mover.hpp"
#include "services/interfaces/workflow/quake3/q3_bot_usercmd.hpp"

#include "q3_test_scene.hpp"

#include <gtest/gtest.h>

namespace impl = sdl3cpp::services::impl;
namespace q3   = sdl3cpp::q3;

namespace {

constexpr float kDt = 1.0f / 60.0f;

nlohmann::json BotAt(float x, float y, float z) {
    nlohmann::json bot;
    bot["pos"] = nlohmann::json::array({x, y, z});
    return bot;
}

glm::vec3 PosOf(const nlohmann::json& bot) {
    return glm::vec3(bot["pos"][0].get<float>(), bot["pos"][1].get<float>(),
                     bot["pos"][2].get<float>());
}

/// A floor centred on the origin, its top surface at y = 0.
void AddFloor(q3test::Scene& scene) {
    scene.Add(btVector3(0.f, -1.f, 0.f), btVector3(20.f, 1.f, 20.f));
}

}  // namespace

TEST(BotMove, ABotDroppedInTheAirLandsOnTheFloor) {
    q3test::Scene scene;
    AddFloor(scene);

    auto bot = BotAt(0.f, 5.f, 0.f);
    for (int frame = 0; frame < 300; ++frame) {
        impl::MoveBotThroughPmove(bot, q3::Q3UserCmd{}, &scene.world, kDt);
    }

    // Feet rest on the floor, so the origin sits a player half-height up.
    EXPECT_NEAR(PosOf(bot).y, -q3::kPlayerFeet, 0.05f);
    EXPECT_TRUE(bot.value("on_ground", false));
}

TEST(BotMove, AnIdleBotDoesNotHover) {
    q3test::Scene scene;
    AddFloor(scene);

    auto bot          = BotAt(0.f, 4.f, 0.f);
    const float start = PosOf(bot).y;
    impl::MoveBotThroughPmove(bot, q3::Q3UserCmd{}, &scene.world, kDt);
    EXPECT_LT(PosOf(bot).y, start);
}

TEST(BotMove, ABotWalkingAtAWallStopsAtIt) {
    q3test::Scene scene;
    AddFloor(scene);
    // A wall across the bot's path at x = 3.
    scene.Add(btVector3(4.f, 2.f, 0.f), btVector3(1.f, 2.f, 20.f));

    auto bot = BotAt(0.f, -q3::kPlayerFeet, 0.f);
    for (int frame = 0; frame < 400; ++frame) {
        const glm::vec3 wish(1.f, 0.f, 0.f);
        const float yaw = q3::YawTowards(wish);
        impl::MoveBotThroughPmove(
            bot, q3::BotDirectionToUserCmd(wish, 1.0f, yaw), &scene.world, kDt);
    }

    const glm::vec3 end = PosOf(bot);
    EXPECT_GT(end.x, 0.5f) << "the bot never set off";
    EXPECT_LT(end.x, 3.0f - q3::kPlayerHalfWidth + 0.05f)
        << "the bot walked into or through the wall";
}

TEST(BotMove, ABotWalksTowardsWhereItIsPointed) {
    q3test::Scene scene;
    AddFloor(scene);

    auto bot = BotAt(0.f, -q3::kPlayerFeet, 0.f);
    for (int frame = 0; frame < 120; ++frame) {
        const glm::vec3 wish(0.f, 0.f, -1.f);
        impl::MoveBotThroughPmove(
            bot, q3::BotDirectionToUserCmd(wish, 1.0f, q3::YawTowards(wish)),
            &scene.world, kDt);
    }

    const glm::vec3 end = PosOf(bot);
    EXPECT_LT(end.z, -1.0f);
    EXPECT_NEAR(end.x, 0.0f, 0.3f);
}

TEST(BotUserCmd, GoingStraightAheadIsPureForwardMove) {
    for (float yaw : {0.0f, 1.1f, -2.4f}) {
        const q3::Q3UserCmd cmd =
            q3::BotDirectionToUserCmd(q3::YawForward(yaw), 1.0f, yaw);
        EXPECT_NEAR(cmd.forwardMove, 1.0f, 1e-3f);
        EXPECT_NEAR(cmd.rightMove, 0.0f, 1e-3f);
    }
}

TEST(BotUserCmd, NowhereToGoIsNoMovement) {
    const q3::Q3UserCmd cmd =
        q3::BotDirectionToUserCmd(glm::vec3(0.0f), 1.0f, 0.5f);
    EXPECT_FLOAT_EQ(cmd.forwardMove, 0.0f);
    EXPECT_FLOAT_EQ(cmd.rightMove, 0.0f);
}
