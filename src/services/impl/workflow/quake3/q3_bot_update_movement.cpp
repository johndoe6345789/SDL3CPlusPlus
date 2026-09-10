#include "services/interfaces/workflow/quake3/q3_bot_update_movement.hpp"
#include "services/interfaces/workflow/quake3/q3_nav_pathfinding.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

void UpdateBotChaseMovement(const q3::NavGraph* navGraph,
                            WorkflowContext& context, int botIndex,
                            nlohmann::json& bot, const glm::vec3& playerPos,
                            const BotUpdateParams& params, double dt,
                            int globalFrame) {
    const auto& posJ = bot["pos"];
    glm::vec3 bpos(posJ[0].get<float>(), posJ[1].get<float>(),
                   posJ[2].get<float>());
    if (glm::distance(playerPos, bpos) <= 0.5f) {
        return;
    }

    const std::string pathKey = "q3.bot_path_" + std::to_string(botIndex);
    std::vector<int> path;
    if (const auto* cached =
            context.TryGet<std::shared_ptr<std::vector<int>>>(pathKey)) {
        if (*cached) path = **cached;
    }

    const int lastPlan = bot.value("last_plan_frame", -params.replanFrames - 1);
    const bool needReplan =
        path.empty() || (globalFrame - lastPlan >= params.replanFrames);

    if (navGraph && needReplan) {
        const int botNode    = q3::NearestNavNode(*navGraph, bpos);
        const int playerNode = q3::NearestNavNode(*navGraph, playerPos);
        path                 = q3::FindNavPath(*navGraph, botNode, playerNode);
        bot["last_plan_frame"] = globalFrame;
        context.Set(pathKey, std::make_shared<std::vector<int>>(path));
    }

    glm::vec3 moveTarget = playerPos;
    if (navGraph && path.size() >= 2) {
        const int nextNode = path[1];
        if (nextNode >= 0 &&
            nextNode < static_cast<int>(navGraph->nodes.size())) {
            moveTarget = navGraph->nodes[nextNode].pos;
        }
    }

    const glm::vec3 toTarget = moveTarget - bpos;
    const float toTargetDist = glm::distance(moveTarget, bpos);
    if (toTargetDist > 0.1f) {
        const glm::vec3 dir = toTarget / toTargetDist;
        bpos.x += dir.x * params.moveSpeed * static_cast<float>(dt);
        bpos.z += dir.z * params.moveSpeed * static_cast<float>(dt);

        if (navGraph && path.size() >= 2 && toTargetDist < 0.8f) {
            path.erase(path.begin());
            context.Set(pathKey, std::make_shared<std::vector<int>>(path));
        }
    }

    bot["pos"] = nlohmann::json::array({bpos.x, bpos.y, bpos.z});
}

}  // namespace sdl3cpp::services::impl
