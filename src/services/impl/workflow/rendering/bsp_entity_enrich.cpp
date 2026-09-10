#include "services/interfaces/workflow/rendering/bsp_entity_enrich.hpp"
#include "services/interfaces/workflow/rendering/bsp_entity_classification.hpp"
#include "services/interfaces/workflow/rendering/bsp_entity_origin.hpp"
#include "services/interfaces/workflow/rendering/bsp_q3_coordinates.hpp"

#include <cstdlib>

namespace sdl3cpp::services::impl {

namespace {

/// Attaches `model_index`/`bounds` when `model` is a `"*N"` brush reference.
void ApplyEntityModelBounds(nlohmann::json& ent,
                            const std::map<std::string, std::string>& values,
                            const std::vector<BspModel>& models, float scale) {
    auto modelIt = values.find("model");
    if (modelIt == values.end() || modelIt->second.size() <= 1 ||
        modelIt->second[0] != '*') {
        return;
    }
    const int modelIndex = std::atoi(modelIt->second.c_str() + 1);
    if (modelIndex >= 0 && static_cast<size_t>(modelIndex) < models.size()) {
        ent["model_index"] = modelIndex;
        ent["bounds"] =
            ConvertModelBounds(models[static_cast<size_t>(modelIndex)], scale);
    }
}

}  // namespace

void EnrichEntity(
    nlohmann::json& ent, const std::map<std::string, std::string>& values,
    const std::vector<BspModel>& models, float scale, int classIndex,
    std::unordered_map<std::string, std::array<float, 3>>& targets,
    nlohmann::json& spawn, int& pickupCount, int& jumpPadCount,
    int& teleporterCount) {
    const std::string classname = ent.value("classname", std::string{});
    ent["id"]                   = classname.empty()
                                      ? ent.value("id", std::string{})
                                      : classname + "_" + std::to_string(classIndex);

    ApplyEntityOrigin(ent, values, classname, scale, targets, spawn);
    ApplyEntityModelBounds(ent, values, models, scale);

    if (IsPickupClass(classname)) {
        ent["kind"] = "pickup";
        ++pickupCount;
    } else if (classname == "trigger_push") {
        ent["kind"] = "jump_pad";
        ++jumpPadCount;
    } else if (classname == "trigger_teleport") {
        ent["kind"] = "teleporter";
        ++teleporterCount;
    }
}

}  // namespace sdl3cpp::services::impl
