#include "services/interfaces/workflow/quake3/q3_bot_mover.hpp"

namespace sdl3cpp::services::impl {
namespace {

glm::vec3 ReadVec(const nlohmann::json& value, const glm::vec3& fallback) {
    if (!value.is_array() || value.size() != 3) return fallback;
    return glm::vec3(value[0].get<float>(), value[1].get<float>(),
                     value[2].get<float>());
}

nlohmann::json VecJson(const glm::vec3& v) {
    return nlohmann::json::array({v.x, v.y, v.z});
}

}  // namespace

Q3PlayerState ReadBotMoveState(const nlohmann::json& bot) {
    Q3PlayerState ps;
    if (bot.contains("pos")) {
        ps.origin = ReadVec(bot["pos"], ps.origin);
    }
    if (bot.contains("vel")) {
        ps.velocity = ReadVec(bot["vel"], glm::vec3(0.0f));
    }
    ps.onGround = bot.value("on_ground", false);
    return ps;
}

void WriteBotMoveState(nlohmann::json& bot, const Q3PlayerState& ps) {
    bot["pos"]       = VecJson(ps.origin);
    bot["vel"]       = VecJson(ps.velocity);
    bot["on_ground"] = ps.onGround;
}

void MoveBotThroughPmove(nlohmann::json& bot, const q3::Q3UserCmd& cmd,
                         btDiscreteDynamicsWorld* world, float dt) {
    Q3PlayerState ps = ReadBotMoveState(bot);
    // A bot has no collision object of its own, so there is nothing for
    // its own traces to skip.
    q3::Q3PmoveOne(ps, cmd, world, dt, nullptr);
    WriteBotMoveState(bot, ps);
}

}  // namespace sdl3cpp::services::impl
