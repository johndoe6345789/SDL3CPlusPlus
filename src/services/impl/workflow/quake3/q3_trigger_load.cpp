#include "services/interfaces/workflow/quake3/q3_trigger_load.hpp"

#include "services/interfaces/workflow/quake3/q3_entity_field_parsers.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

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

}  // namespace sdl3cpp::services::impl
