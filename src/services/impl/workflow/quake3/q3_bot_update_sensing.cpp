#include "services/interfaces/workflow/quake3/q3_bot_update_sensing.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

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

}  // namespace sdl3cpp::services::impl
