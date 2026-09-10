#include "services/interfaces/workflow/quake3/q3_trigger_overlap.hpp"

#include <cmath>
#include <string>

namespace sdl3cpp::services::impl {

glm::vec3 ReadQ3TriggerPlayerPos(const WorkflowContext& context) {
    glm::vec3 playerPos(0.f);
    if (const auto* pp = context.TryGet<glm::vec3>("q3.player_pos")) {
        return *pp;
    }
    const auto camState =
        context.Get<nlohmann::json>("camera.state", nlohmann::json::object());
    if (camState.contains("position") && camState["position"].is_array()) {
        const auto& cp = camState["position"];
        if (cp.size() >= 3) {
            playerPos = {cp[0].get<float>(), cp[1].get<float>(),
                         cp[2].get<float>()};
        }
    }
    return playerPos;
}

void ApplyQ3TriggerOverlaps(WorkflowContext& context,
                            const nlohmann::json& triggerList,
                            const nlohmann::json* destIndex,
                            const glm::vec3& playerPos) {
    constexpr float kOverlapDist = 1.5f;

    for (const auto& t : triggerList) {
        const auto& oj = t["origin"];
        const glm::vec3 tOrigin(oj[0].get<float>(), oj[1].get<float>(),
                                oj[2].get<float>());

        const glm::vec3 diff = playerPos - tOrigin;
        const float d2 = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (d2 > kOverlapDist * kOverlapDist) continue;

        const std::string cls = t["classname"].get<std::string>();

        if (cls == "trigger_push") {
            const std::string tgtName = t.value("target", std::string{});
            if (!tgtName.empty() && destIndex && destIndex->contains(tgtName)) {
                const auto& dj = (*destIndex)[tgtName];
                const glm::vec3 targetPos(
                    dj[0].get<float>(), dj[1].get<float>(), dj[2].get<float>());

                const glm::vec3 toTarget = targetPos - playerPos;
                const float height       = std::max(toTarget.y, 1.0f);
                constexpr float kGravity = 20.f;
                const float vy           = std::sqrt(2.f * kGravity * height);
                const float time         = vy / kGravity;

                const glm::vec3 launchVel(toTarget.x / time, vy,
                                          toTarget.z / time);

                context.Set("q3.player_velocity_override", launchVel);
                context.Set("q3.player_on_jump_pad", true);
            }
        } else if (cls == "trigger_teleport") {
            const std::string tgtName = t.value("target", std::string{});
            if (!tgtName.empty() && destIndex && destIndex->contains(tgtName)) {
                const auto& dj = (*destIndex)[tgtName];
                const glm::vec3 destPos(dj[0].get<float>(), dj[1].get<float>(),
                                        dj[2].get<float>());
                context.Set("q3.player_teleport_dest", destPos);
            }
        } else if (cls == "trigger_hurt") {
            const float dmg      = t.value("dmg", 5.f);
            const float existing = context.Get<float>("q3.pending_damage", 0.f);
            context.Set("q3.pending_damage", existing + dmg);
        }
    }
}

}  // namespace sdl3cpp::services::impl
