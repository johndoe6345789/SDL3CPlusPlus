#include "services/interfaces/workflow/rendering/bsp_entity_json_builder.hpp"
#include "services/interfaces/workflow/rendering/bsp_entity_enrich.hpp"
#include "services/interfaces/workflow/rendering/bsp_q3_coordinates.hpp"

#include <unordered_map>

namespace sdl3cpp::services::impl {

BspEntitiesResult BuildBspEntitiesJson(
    const std::vector<std::map<std::string, std::string>>& parsedEntities,
    const std::vector<BspModel>& models, float scale) {
    BspEntitiesResult result;
    std::unordered_map<std::string, std::array<float, 3>> targets;
    std::map<std::string, int> classCounts;

    for (size_t i = 0; i < parsedEntities.size(); ++i) {
        const auto& values = parsedEntities[i];
        nlohmann::json ent = nlohmann::json::object();
        for (const auto& [key, value] : values) {
            ent[key] = value;
        }
        ent["id"] = "ent_" + std::to_string(i);  // overwritten below if named

        const std::string classname = ent.value("classname", std::string{});
        EnrichEntity(ent, values, models, scale, classCounts[classname]++,
                     targets, result.spawn, result.pickupCount,
                     result.jumpPadCount, result.teleporterCount);
        result.entities.push_back(std::move(ent));
    }

    for (auto& ent : result.entities) {
        const std::string target = ent.value("target", std::string{});
        if (target.empty()) {
            continue;
        }
        auto targetIt = targets.find(target);
        if (targetIt != targets.end()) {
            ent["target_position"] = PointJson(targetIt->second);
        }
    }

    if (result.spawn.is_null()) {
        result.spawn = {{"x", 0.0f}, {"y", 5.0f}, {"z", 0.0f}, {"angle", 0.0f}};
    }
    return result;
}

}  // namespace sdl3cpp::services::impl
