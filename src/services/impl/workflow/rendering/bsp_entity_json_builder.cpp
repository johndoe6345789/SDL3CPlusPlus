#include "services/interfaces/workflow/rendering/bsp_entity_json_builder.hpp"
#include "services/interfaces/workflow/rendering/bsp_entity_lump_parser.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <unordered_map>

namespace sdl3cpp::services::impl {
namespace {

bool HasPrefix(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

bool IsPickupClass(const std::string& classname) {
    return HasPrefix(classname, "weapon_") || HasPrefix(classname, "ammo_") ||
           HasPrefix(classname, "item_") || HasPrefix(classname, "holdable_");
}

/// Q3 is Z-up; the engine is Y-up. x'=x, y'=z, z'=-y, all scaled.
std::array<float, 3> ConvertQ3Point(float qx, float qy, float qz, float scale) {
    return {qx * scale, qz * scale, -qy * scale};
}

nlohmann::json PointJson(const std::array<float, 3>& p) {
    return nlohmann::json::array({p[0], p[1], p[2]});
}

nlohmann::json ConvertModelBounds(const BspModel& model, float scale) {
    std::array<float, 3> mn{std::numeric_limits<float>::max(),
                            std::numeric_limits<float>::max(),
                            std::numeric_limits<float>::max()};
    std::array<float, 3> mx{std::numeric_limits<float>::lowest(),
                            std::numeric_limits<float>::lowest(),
                            std::numeric_limits<float>::lowest()};

    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                const auto p =
                    ConvertQ3Point(x ? model.maxs[0] : model.mins[0],
                                   y ? model.maxs[1] : model.mins[1],
                                   z ? model.maxs[2] : model.mins[2], scale);
                for (int axis = 0; axis < 3; ++axis) {
                    mn[axis] = std::min(mn[axis], p[axis]);
                    mx[axis] = std::max(mx[axis], p[axis]);
                }
            }
        }
    }
    return nlohmann::json{{"min", PointJson(mn)}, {"max", PointJson(mx)}};
}

/// Fills in `id`, `position`, `model_index`/`bounds` and `kind`; records the
/// entity's world position (for later target resolution) and, for the first
/// info_player_deathmatch found, the spawn point.
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

    auto originIt = values.find("origin");
    if (originIt != values.end()) {
        float ox = 0, oy = 0, oz = 0;
        if (ParseBspVec3(originIt->second, ox, oy, oz)) {
            const auto p    = ConvertQ3Point(ox, oy, oz, scale);
            ent["position"] = PointJson(p);

            auto targetNameIt = values.find("targetname");
            if (targetNameIt != values.end()) {
                targets[targetNameIt->second] = p;
            }

            if (classname == "info_player_deathmatch" && spawn.is_null()) {
                spawn        = {{"x", p[0]}, {"y", p[1] + 1.0f}, {"z", p[2]}};
                auto angleIt = values.find("angle");
                float angle  = 0.0f;
                if (angleIt != values.end()) {
                    try {
                        angle = std::stof(angleIt->second);
                    } catch (...) {
                        angle = 0.0f;
                    }
                }
                spawn["angle"] = angle;
            }
        }
    }

    auto modelIt = values.find("model");
    if (modelIt != values.end() && modelIt->second.size() > 1 &&
        modelIt->second[0] == '*') {
        const int modelIndex = std::atoi(modelIt->second.c_str() + 1);
        if (modelIndex >= 0 &&
            static_cast<size_t>(modelIndex) < models.size()) {
            ent["model_index"] = modelIndex;
            ent["bounds"]      = ConvertModelBounds(
                models[static_cast<size_t>(modelIndex)], scale);
        }
    }

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

}  // namespace

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
