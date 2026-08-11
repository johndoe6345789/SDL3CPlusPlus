#include "services/interfaces/workflow/quake3/workflow_q3_bots_update_step.hpp"
#include "services/interfaces/workflow/quake3/q3_nav_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <queue>
#include <string>
#include <vector>
#include <memory>

namespace sdl3cpp::services::impl {

// ----------------------------------------------------------------
// A* over NavGraph
// Returns node-index path from startNode to goalNode, or empty on failure.
// ----------------------------------------------------------------
namespace {

std::vector<int> AStar(const sdl3cpp::q3::NavGraph& graph, int startNode, int goalNode) {
    if (startNode < 0 || goalNode < 0 ||
        startNode >= (int)graph.nodes.size() ||
        goalNode  >= (int)graph.nodes.size()) {
        return {};
    }
    if (startNode == goalNode) return {startNode};

    const int N = (int)graph.nodes.size();
    std::vector<float> gCost(N, 1e9f);
    std::vector<int>   parent(N, -1);

    auto heuristic = [&](int a, int b) {
        const glm::vec3 d = graph.nodes[a].pos - graph.nodes[b].pos;
        return std::sqrt(d.x*d.x + d.y*d.y + d.z*d.z);
    };

    // Min-heap: (fCost, nodeIdx)
    using Entry = std::pair<float, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;

    gCost[startNode] = 0.f;
    open.push({heuristic(startNode, goalNode), startNode});

    while (!open.empty()) {
        const auto [f, cur] = open.top(); open.pop();
        if (cur == goalNode) break;

        for (int nb : graph.nodes[cur].neighbors) {
            const glm::vec3 d = graph.nodes[nb].pos - graph.nodes[cur].pos;
            const float edgeCost = std::sqrt(d.x*d.x + d.y*d.y + d.z*d.z);
            const float newG = gCost[cur] + edgeCost;
            if (newG < gCost[nb]) {
                gCost[nb]  = newG;
                parent[nb] = cur;
                open.push({newG + heuristic(nb, goalNode), nb});
            }
        }
    }

    if (parent[goalNode] == -1 && goalNode != startNode) return {};

    // Reconstruct path
    std::vector<int> path;
    for (int cur = goalNode; cur != -1; cur = parent[cur])
        path.push_back(cur);
    std::reverse(path.begin(), path.end());
    return path;
}

// Find the nearest nav node to a world position
int NearestNode(const sdl3cpp::q3::NavGraph& graph, const glm::vec3& pos) {
    int best = -1;
    float bestD2 = 1e9f;
    for (int i = 0; i < (int)graph.nodes.size(); ++i) {
        const glm::vec3 d = graph.nodes[i].pos - pos;
        const float d2 = d.x*d.x + d.y*d.y + d.z*d.z;
        if (d2 < bestD2) { bestD2 = d2; best = i; }
    }
    return best;
}

}  // namespace

// ----------------------------------------------------------------

WorkflowQ3BotsUpdateStep::WorkflowQ3BotsUpdateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3BotsUpdateStep::GetPluginId() const { return "q3.bots.update"; }

void WorkflowQ3BotsUpdateStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    auto* botsPtr = context.TryGet<nlohmann::json>("q3.bots");
    if (!botsPtr || !botsPtr->is_array() || botsPtr->empty()) return;

    WorkflowStepParameterResolver params;
    auto getNum = [&](const char* k, float def) -> float {
        const auto* p = params.FindParameter(step, k);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? (float)p->numberValue : def;
    };

    const float chaseRange  = getNum("chase_range",    20.0f);
    const float shootRange  = getNum("shoot_range",     6.0f);
    const float moveSpeed   = getNum("move_speed",      3.0f);
    const int   legIdle     = (int)getNum("leg_idle",   162);  // Q3 keel defaults
    const int   legRun      = (int)getNum("leg_run",    167);
    const int   legRunCnt   = (int)getNum("leg_run_cnt",  8);
    const int   torsoStand  = (int)getNum("torso_stand", 101);
    const int   torsoAtk    = (int)getNum("torso_attack",107);
    const int   torsoAtkCnt = (int)getNum("torso_atk_cnt", 6);
    const int   shootInterv = (int)getNum("shoot_interval", 30);  // frames
    const int   replanRate  = (int)getNum("replan_frames",  60);  // A* replan interval

    // Player position: prefer explicit q3.player_pos, fall back to camera.state
    const auto camState = context.Get<nlohmann::json>("camera.state", nlohmann::json::object());
    glm::vec3 playerPos(0.0f);
    if (camState.contains("position") && camState["position"].is_array()) {
        const auto& cp = camState["position"];
        if (cp.size() >= 3)
            playerPos = {cp[0].get<float>(), cp[1].get<float>(), cp[2].get<float>()};
    }
    if (const auto* pp = context.TryGet<glm::vec3>("q3.player_pos"))
        playerPos = *pp;

    const double dt          = context.GetDouble("frame.delta_time", 0.016);
    const double elapsed     = context.GetDouble("frame.elapsed",    0.0);
    const int    globalFrame = (int)(elapsed * 60.0);  // ~60fps frame counter

    // Nav graph — may be null when q3.nav.build hasn't run yet
    sdl3cpp::q3::NavGraph* navGraph = nullptr;
    if (const auto* ngp = context.TryGet<sdl3cpp::q3::NavGraphPtr>("q3.nav_graph"))
        if (*ngp && !(*ngp)->nodes.empty())
            navGraph = ngp->get();

    // Physics world for line-of-sight raycasts
    auto* world = context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);

    nlohmann::json bots  = *botsPtr;
    nlohmann::json shots = nlohmann::json::array();

    for (int botIdx = 0; botIdx < (int)bots.size(); ++botIdx) {
        auto& bot = bots[botIdx];
        if (bot.value("state", std::string{}) == "dead") continue;

        const auto& posJ = bot["pos"];
        glm::vec3 bpos(posJ[0].get<float>(), posJ[1].get<float>(), posJ[2].get<float>());

        const glm::vec3 toPlayer = playerPos - bpos;
        const float dist = std::sqrt(toPlayer.x * toPlayer.x +
                                     toPlayer.y * toPlayer.y +
                                     toPlayer.z * toPlayer.z);

        // Face player: yaw = atan2 in XZ plane (Y-up world)
        const float targetYaw = std::atan2(toPlayer.x, toPlayer.z);
        bot["yaw"] = targetYaw;

        // Line-of-sight check via physics raycast
        bool canSeePlayer = false;
        if (world && dist > 0.f) {
            btVector3 from(bpos.x, bpos.y, bpos.z);
            btVector3 to(playerPos.x, playerPos.y, playerPos.z);
            btCollisionWorld::ClosestRayResultCallback cb(from, to);
            world->rayTest(from, to, cb);
            canSeePlayer = !cb.hasHit();
        } else {
            canSeePlayer = (dist < chaseRange);
        }

        // State transitions
        if (dist < shootRange && canSeePlayer) {
            bot["state"] = "shoot";
        } else if (dist < chaseRange) {
            bot["state"] = "chase";
        } else {
            bot["state"] = "idle";
        }

        const std::string state = bot["state"].get<std::string>();

        // ----------------------------------------------------------------
        // Movement with A* path following (falls back to direct if no graph)
        // ----------------------------------------------------------------
        if (state == "chase" && dist > 0.5f) {
            const std::string pathKey = "q3.bot_path_" + std::to_string(botIdx);

            // Load cached path
            std::vector<int> path;
            if (const auto* cached = context.TryGet<std::shared_ptr<std::vector<int>>>(pathKey))
                if (*cached) path = **cached;

            // Re-plan when path empty or stale
            const int lastPlan  = bot.value("last_plan_frame", -replanRate - 1);
            const bool needReplan = path.empty() || (globalFrame - lastPlan >= replanRate);

            if (navGraph && needReplan) {
                const int botNode    = NearestNode(*navGraph, bpos);
                const int playerNode = NearestNode(*navGraph, playerPos);
                path = AStar(*navGraph, botNode, playerNode);
                bot["last_plan_frame"] = globalFrame;
                context.Set(pathKey, std::make_shared<std::vector<int>>(path));
            }

            // Steer toward path[1] (next waypoint) or directly at player
            glm::vec3 moveTarget = playerPos;
            if (navGraph && path.size() >= 2) {
                const int nextNode = path[1];
                if (nextNode >= 0 && nextNode < (int)navGraph->nodes.size())
                    moveTarget = navGraph->nodes[nextNode].pos;
            }

            const glm::vec3 toTarget = moveTarget - bpos;
            const float td = std::sqrt(toTarget.x*toTarget.x +
                                       toTarget.y*toTarget.y +
                                       toTarget.z*toTarget.z);
            if (td > 0.1f) {
                const glm::vec3 dir = toTarget / td;
                bpos.x += dir.x * moveSpeed * (float)dt;
                bpos.z += dir.z * moveSpeed * (float)dt;

                // Pop waypoint when close enough to advance path
                if (navGraph && path.size() >= 2 && td < 0.8f) {
                    path.erase(path.begin());
                    context.Set(pathKey, std::make_shared<std::vector<int>>(path));
                }
            }

            bot["pos"] = nlohmann::json::array({bpos.x, bpos.y, bpos.z});
        }

        // Animation frame selection
        const double fps = 15.0;
        const int baseFrame = (int)(elapsed * fps);
        if (state == "chase") {
            bot["leg_frame"]   = legRun + (legRunCnt > 0 ? (baseFrame % legRunCnt) : 0);
            bot["torso_frame"] = torsoStand;
        } else if (state == "shoot") {
            bot["leg_frame"]   = legIdle;
            bot["torso_frame"] = torsoAtk + (torsoAtkCnt > 0 ? (baseFrame % torsoAtkCnt) : 0);

            // Fire shot
            const int lastShot = bot.value("last_shot", 0);
            if (globalFrame >= lastShot + shootInterv) {
                bot["last_shot"] = globalFrame;
                shots.push_back({
                    {"bot_id", bot["id"]},
                    {"from",   bot["pos"]},
                    {"to",     nlohmann::json::array({playerPos.x, playerPos.y, playerPos.z})}
                });
            }
        } else {
            bot["leg_frame"]   = legIdle;
            bot["torso_frame"] = torsoStand;
        }
    }

    context.Set("q3.bots", bots);
    if (!shots.empty()) context.Set("q3.bot_shots", shots);
}

}  // namespace sdl3cpp::services::impl
