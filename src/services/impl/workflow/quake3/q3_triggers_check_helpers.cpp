#include "services/interfaces/workflow/quake3/q3_triggers_check_helpers.hpp"

#include <cmath>
#include <cstdlib>
#include <string>

namespace sdl3cpp::services::impl {

float EntFloat(const nlohmann::json& ent, const char* key, float def) {
    if (!ent.contains(key)) return def;
    const auto& v = ent[key];
    if (v.is_number()) return v.get<float>();
    if (v.is_string()) {
        try {
            return std::stof(v.get<std::string>());
        } catch (...) {}
    }
    return def;
}

glm::vec3 ParseOrigin(const std::string& s, float scale) {
    float x = 0.f, y = 0.f, z = 0.f;
    std::sscanf(s.c_str(), "%f %f %f", &x, &y, &z);
    return glm::vec3(x * scale, y * scale, z * scale);
}

void LoadQ3TriggersIfNeeded(WorkflowContext& context,
                            const std::shared_ptr<ILogger>& logger) {
    if (context.GetBool("q3.triggers_loaded", false)) return;

    const auto* entities = context.TryGet<nlohmann::json>("bsp.entities");
    if (!entities || !entities->is_array()) {
        context.Set("q3.triggers_loaded", true);
        return;
    }

    // Build index: targetname -> origin (for destination lookups).
    nlohmann::json triggerList = nlohmann::json::array();
    nlohmann::json destIndex   = nlohmann::json::object();

    for (const auto& ent : *entities) {
        const std::string cls = ent.value("classname", std::string{});

        // Record destination entities.
        if (cls == "target_position" || cls == "misc_teleporter_dest") {
            const std::string tname = ent.value("targetname", std::string{});
            if (!tname.empty() && ent.contains("origin") &&
                ent["origin"].is_string()) {
                glm::vec3 o = ParseOrigin(ent["origin"].get<std::string>());
                destIndex[tname] = nlohmann::json::array({o.x, o.y, o.z});
            }
        }

        if (cls != "trigger_push" && cls != "trigger_teleport" &&
            cls != "trigger_hurt") {
            continue;
        }

        glm::vec3 origin(0.f);
        if (ent.contains("origin") && ent["origin"].is_string()) {
            origin = ParseOrigin(ent["origin"].get<std::string>());
        }

        nlohmann::json t;
        t["classname"] = cls;
        t["origin"]    = nlohmann::json::array({origin.x, origin.y, origin.z});
        t["target"]    = ent.value("target", std::string{});
        t["dmg"]       = EntFloat(ent, "dmg", 5.f);
        triggerList.push_back(t);
    }

    context.Set("q3.trigger_list", triggerList);
    context.Set("q3.trigger_dest_index", destIndex);
    context.Set("q3.triggers_loaded", true);

    if (logger) {
        logger->Info("q3.triggers.check: parsed " +
                     std::to_string(triggerList.size()) + " triggers");
    }
}

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
