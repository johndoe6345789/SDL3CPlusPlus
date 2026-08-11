#include "services/interfaces/workflow/quake3/workflow_q3_triggers_check_step.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

namespace {

// BSP entity numeric fields are stored as JSON strings.
float EntFloat(const nlohmann::json& ent, const char* key, float def) {
    if (!ent.contains(key)) return def;
    const auto& v = ent[key];
    if (v.is_number()) return v.get<float>();
    if (v.is_string()) { try { return std::stof(v.get<std::string>()); } catch (...) {} }
    return def;
}

// Parsed representation of a trigger entity
struct TriggerEnt {
    std::string classname;
    glm::vec3   origin{};
    float       radius{1.5f};   // overlap distance
    std::string target;         // target entity name (for jump pad / teleport)
    float       dmg{5.f};       // for trigger_hurt
};

// Parse "x y z" string into glm::vec3 with optional Q3 unit scale
glm::vec3 ParseOrigin(const std::string& s, float scale = 0.03125f) {
    float x = 0.f, y = 0.f, z = 0.f;
    std::sscanf(s.c_str(), "%f %f %f", &x, &y, &z);
    return glm::vec3(x * scale, y * scale, z * scale);
}

}  // namespace

WorkflowQ3TriggersCheckStep::WorkflowQ3TriggersCheckStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3TriggersCheckStep::GetPluginId() const { return "q3.triggers.check"; }

void WorkflowQ3TriggersCheckStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    if (context.GetBool("q3.player_dead", false)) return;

    // ----------------------------------------------------------------
    // One-time parse of trigger entities
    // ----------------------------------------------------------------
    if (!context.GetBool("q3.triggers_loaded", false)) {
        const auto* entities = context.TryGet<nlohmann::json>("bsp.entities");
        if (!entities || !entities->is_array()) {
            context.Set("q3.triggers_loaded", true);
            return;
        }

        // Build index: targetname → origin (for destination lookups)
        // Store as shared nlohmann::json in context
        nlohmann::json triggerList  = nlohmann::json::array();
        nlohmann::json destIndex    = nlohmann::json::object();  // targetname → [x,y,z]

        for (const auto& ent : *entities) {
            const std::string cls = ent.value("classname", std::string{});

            // Record destination entities
            if (cls == "target_position" || cls == "misc_teleporter_dest") {
                const std::string tname = ent.value("targetname", std::string{});
                if (!tname.empty() && ent.contains("origin") && ent["origin"].is_string()) {
                    glm::vec3 o = ParseOrigin(ent["origin"].get<std::string>());
                    destIndex[tname] = nlohmann::json::array({o.x, o.y, o.z});
                }
            }

            if (cls != "trigger_push" && cls != "trigger_teleport" && cls != "trigger_hurt") continue;

            glm::vec3 origin(0.f);
            if (ent.contains("origin") && ent["origin"].is_string())
                origin = ParseOrigin(ent["origin"].get<std::string>());

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

        if (logger_)
            logger_->Info("q3.triggers.check: parsed " + std::to_string(triggerList.size()) + " triggers");
    }

    // ----------------------------------------------------------------
    // Per-frame overlap checks
    // ----------------------------------------------------------------
    const auto* triggerListPtr = context.TryGet<nlohmann::json>("q3.trigger_list");
    if (!triggerListPtr || !triggerListPtr->is_array()) return;

    const auto* destIdxPtr = context.TryGet<nlohmann::json>("q3.trigger_dest_index");

    glm::vec3 playerPos(0.f);
    if (const auto* pp = context.TryGet<glm::vec3>("q3.player_pos")) {
        playerPos = *pp;
    } else {
        const auto camState = context.Get<nlohmann::json>("camera.state", nlohmann::json::object());
        if (camState.contains("position") && camState["position"].is_array()) {
            const auto& cp = camState["position"];
            if (cp.size() >= 3)
                playerPos = {cp[0].get<float>(), cp[1].get<float>(), cp[2].get<float>()};
        }
    }

    constexpr float kOverlapDist = 1.5f;

    for (const auto& t : *triggerListPtr) {
        const auto& oj = t["origin"];
        const glm::vec3 tOrigin(oj[0].get<float>(), oj[1].get<float>(), oj[2].get<float>());

        const glm::vec3 diff = playerPos - tOrigin;
        const float d2 = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
        if (d2 > kOverlapDist * kOverlapDist) continue;

        const std::string cls = t["classname"].get<std::string>();

        if (cls == "trigger_push") {
            // Find target_position from dest index
            const std::string tgtName = t.value("target", std::string{});
            if (!tgtName.empty() && destIdxPtr && destIdxPtr->contains(tgtName)) {
                const auto& dj = (*destIdxPtr)[tgtName];
                const glm::vec3 targetPos(dj[0].get<float>(), dj[1].get<float>(), dj[2].get<float>());

                const glm::vec3 toTarget = targetPos - playerPos;
                const float height = std::max(toTarget.y, 1.0f);
                constexpr float kGravity = 20.f;
                const float vy    = std::sqrt(2.f * kGravity * height);
                const float time  = vy / kGravity;

                const glm::vec3 launchVel(
                    toTarget.x / time,
                    vy,
                    toTarget.z / time
                );

                context.Set("q3.player_velocity_override", launchVel);
                context.Set("q3.player_on_jump_pad", true);
            }
        } else if (cls == "trigger_teleport") {
            const std::string tgtName = t.value("target", std::string{});
            if (!tgtName.empty() && destIdxPtr && destIdxPtr->contains(tgtName)) {
                const auto& dj = (*destIdxPtr)[tgtName];
                const glm::vec3 destPos(dj[0].get<float>(), dj[1].get<float>(), dj[2].get<float>());
                context.Set("q3.player_teleport_dest", destPos);
            }
        } else if (cls == "trigger_hurt") {
            const float dmg = t.value("dmg", 5.f);
            const float existing = context.Get<float>("q3.pending_damage", 0.f);
            context.Set("q3.pending_damage", existing + dmg);
        }
    }
}

}  // namespace sdl3cpp::services::impl
