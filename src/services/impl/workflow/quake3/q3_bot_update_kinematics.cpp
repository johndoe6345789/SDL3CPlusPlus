#include "services/interfaces/workflow/quake3/q3_bot_update_kinematics.hpp"
#include "services/interfaces/workflow/quake3/q3_nav_nearest.hpp"
#include "services/interfaces/workflow/quake3/q3_nav_pathfinding.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <cmath>
#include <memory>

namespace sdl3cpp::services::impl {
namespace {

float NumberParameter(const WorkflowStepParameterResolver& params,
                      const WorkflowStepDefinition& step, const char* key,
                      float fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isNumber = p && p->type == WorkflowParameterValue::Type::Number;
    return isNumber ? static_cast<float>(p->numberValue) : fallback;
}

float Distance(const glm::vec3& a, const glm::vec3& b) {
    const glm::vec3 d = a - b;
    return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}

}  // namespace

BotUpdateParams ReadBotUpdateParams(const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    BotUpdateParams p;
    p.chaseRange = NumberParameter(params, step, "chase_range", p.chaseRange);
    p.shootRange = NumberParameter(params, step, "shoot_range", p.shootRange);
    p.moveSpeed  = NumberParameter(params, step, "move_speed", p.moveSpeed);
    p.legIdle    = static_cast<int>(NumberParameter(
        params, step, "leg_idle", static_cast<float>(p.legIdle)));
    p.legRun     = static_cast<int>(
        NumberParameter(params, step, "leg_run", static_cast<float>(p.legRun)));
    p.legRunCount         = static_cast<int>(NumberParameter(
        params, step, "leg_run_cnt", static_cast<float>(p.legRunCount)));
    p.torsoStand          = static_cast<int>(NumberParameter(
        params, step, "torso_stand", static_cast<float>(p.torsoStand)));
    p.torsoAttack         = static_cast<int>(NumberParameter(
        params, step, "torso_attack", static_cast<float>(p.torsoAttack)));
    p.torsoAttackCount    = static_cast<int>(NumberParameter(
        params, step, "torso_atk_cnt", static_cast<float>(p.torsoAttackCount)));
    p.shootIntervalFrames = static_cast<int>(
        NumberParameter(params, step, "shoot_interval",
                        static_cast<float>(p.shootIntervalFrames)));
    p.replanFrames = static_cast<int>(NumberParameter(
        params, step, "replan_frames", static_cast<float>(p.replanFrames)));
    return p;
}

bool HasLineOfSightToPlayer(btDiscreteDynamicsWorld* world,
                            const glm::vec3& from, const glm::vec3& to,
                            float distance, float chaseRange) {
    if (world && distance > 0.0f) {
        const btVector3 btFrom(from.x, from.y, from.z);
        const btVector3 btTo(to.x, to.y, to.z);
        btCollisionWorld::ClosestRayResultCallback rayResult(btFrom, btTo);
        world->rayTest(btFrom, btTo, rayResult);
        return !rayResult.hasHit();
    }
    return distance < chaseRange;
}

std::string DetermineBotState(float distance, bool canSeePlayer,
                              float shootRange, float chaseRange) {
    if (distance < shootRange && canSeePlayer) {
        return "shoot";
    }
    if (distance < chaseRange) {
        return "chase";
    }
    return "idle";
}

void UpdateBotChaseMovement(const q3::NavGraph* navGraph,
                            WorkflowContext& context, int botIndex,
                            nlohmann::json& bot, const glm::vec3& playerPos,
                            const BotUpdateParams& params, double dt,
                            int globalFrame) {
    const auto& posJ = bot["pos"];
    glm::vec3 bpos(posJ[0].get<float>(), posJ[1].get<float>(),
                   posJ[2].get<float>());
    if (Distance(playerPos, bpos) <= 0.5f) {
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
    const float toTargetDist = Distance(moveTarget, bpos);
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

void SelectBotAnimationAndFire(nlohmann::json& bot, const std::string& state,
                               double elapsedSeconds, int globalFrame,
                               const glm::vec3& playerPos,
                               const BotUpdateParams& params,
                               nlohmann::json& shots) {
    constexpr double kAnimFps = 15.0;
    const int baseFrame       = static_cast<int>(elapsedSeconds * kAnimFps);

    if (state == "chase") {
        bot["leg_frame"] =
            params.legRun +
            (params.legRunCount > 0 ? (baseFrame % params.legRunCount) : 0);
        bot["torso_frame"] = params.torsoStand;
        return;
    }

    if (state == "shoot") {
        bot["leg_frame"] = params.legIdle;
        bot["torso_frame"] =
            params.torsoAttack + (params.torsoAttackCount > 0
                                      ? (baseFrame % params.torsoAttackCount)
                                      : 0);

        const int lastShot = bot.value("last_shot", 0);
        if (globalFrame >= lastShot + params.shootIntervalFrames) {
            bot["last_shot"] = globalFrame;
            shots.push_back(
                {{"bot_id", bot["id"]},
                 {"from", bot["pos"]},
                 {"to", nlohmann::json::array(
                            {playerPos.x, playerPos.y, playerPos.z})}});
        }
        return;
    }

    bot["leg_frame"]   = params.legIdle;
    bot["torso_frame"] = params.torsoStand;
}

glm::vec3 ReadBotUpdatePlayerPosition(const WorkflowContext& context) {
    const auto camState =
        context.Get<nlohmann::json>("camera.state", nlohmann::json::object());
    glm::vec3 playerPos(0.0f);
    if (camState.contains("position") && camState["position"].is_array()) {
        const auto& cp = camState["position"];
        if (cp.size() >= 3) {
            playerPos = {cp[0].get<float>(), cp[1].get<float>(),
                         cp[2].get<float>()};
        }
    }
    if (const auto* pp = context.TryGet<glm::vec3>("q3.player_pos")) {
        playerPos = *pp;
    }
    return playerPos;
}

const q3::NavGraph* ReadBotUpdateNavGraph(const WorkflowContext& context) {
    if (const auto* ptr = context.TryGet<q3::NavGraphPtr>("q3.nav_graph")) {
        if (*ptr && !(*ptr)->nodes.empty()) {
            return ptr->get();
        }
    }
    return nullptr;
}

void UpdateOneBot(int botIndex, nlohmann::json& bot, const glm::vec3& playerPos,
                  const q3::NavGraph* navGraph, btDiscreteDynamicsWorld* world,
                  const BotUpdateParams& params, WorkflowContext& context,
                  double dt, double elapsedSeconds, int globalFrame,
                  nlohmann::json& shots) {
    const auto& posJ = bot["pos"];
    const glm::vec3 bpos(posJ[0].get<float>(), posJ[1].get<float>(),
                         posJ[2].get<float>());
    const glm::vec3 toPlayer = playerPos - bpos;
    const float dist         = Distance(playerPos, bpos);

    bot["yaw"] = std::atan2(toPlayer.x, toPlayer.z);

    const bool canSeePlayer =
        HasLineOfSightToPlayer(world, bpos, playerPos, dist, params.chaseRange);
    const std::string state = DetermineBotState(
        dist, canSeePlayer, params.shootRange, params.chaseRange);
    bot["state"] = state;

    if (state == "chase") {
        UpdateBotChaseMovement(navGraph, context, botIndex, bot, playerPos,
                               params, dt, globalFrame);
    }
    SelectBotAnimationAndFire(bot, state, elapsedSeconds, globalFrame,
                              playerPos, params, shots);
}

}  // namespace sdl3cpp::services::impl
