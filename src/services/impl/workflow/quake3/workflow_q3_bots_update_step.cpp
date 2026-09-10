#include "services/interfaces/workflow/quake3/workflow_q3_bots_update_step.hpp"
#include "services/interfaces/workflow/quake3/q3_bot_update_kinematics.hpp"
#include "services/interfaces/workflow/quake3/q3_bot_update_params.hpp"
#include "services/interfaces/workflow/quake3/q3_bot_update_sensing.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3BotsUpdateStep::WorkflowQ3BotsUpdateStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3BotsUpdateStep::GetPluginId() const {
    return "q3.bots.update";
}

void WorkflowQ3BotsUpdateStep::Execute(const WorkflowStepDefinition& step,
                                       WorkflowContext& context) {
    auto* botsPtr = context.TryGet<nlohmann::json>("q3.bots");
    if (!botsPtr || !botsPtr->is_array() || botsPtr->empty()) {
        return;
    }

    const BotUpdateParams params = ReadBotUpdateParams(step);
    const glm::vec3 playerPos    = ReadBotUpdatePlayerPosition(context);
    const q3::NavGraph* navGraph = ReadBotUpdateNavGraph(context);
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);

    const double dt       = context.GetDouble("frame.delta_time", 0.016);
    const double elapsed  = context.GetDouble("frame.elapsed", 0.0);
    const int globalFrame = static_cast<int>(elapsed * 60.0);  // ~60fps

    nlohmann::json bots  = *botsPtr;
    nlohmann::json shots = nlohmann::json::array();

    for (int botIdx = 0; botIdx < static_cast<int>(bots.size()); ++botIdx) {
        auto& bot = bots[botIdx];
        if (bot.value("state", std::string{}) == "dead") {
            continue;
        }
        UpdateOneBot(botIdx, bot, playerPos, navGraph, world, params, context,
                     dt, elapsed, globalFrame, shots);
    }

    context.Set("q3.bots", bots);
    if (!shots.empty()) {
        context.Set("q3.bot_shots", shots);
    }
}

}  // namespace sdl3cpp::services::impl
