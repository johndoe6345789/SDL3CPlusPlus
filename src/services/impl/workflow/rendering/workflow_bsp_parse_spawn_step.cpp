#include "services/interfaces/workflow/rendering/workflow_bsp_parse_spawn_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

namespace {

bool HasPrefix(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

bool IsPickupClass(const std::string& classname) {
    return HasPrefix(classname, "weapon_") ||
           HasPrefix(classname, "ammo_") ||
           HasPrefix(classname, "item_") ||
           HasPrefix(classname, "holdable_");
}

void SkipWhitespace(const std::string& text, size_t& pos) {
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
}

bool ReadQuotedToken(const std::string& text, size_t& pos, std::string& out) {
    SkipWhitespace(text, pos);
    if (pos >= text.size() || text[pos] != '"') return false;
    ++pos;

    out.clear();
    while (pos < text.size()) {
        const char c = text[pos++];
        if (c == '"') return true;
        if (c == '\\' && pos < text.size()) {
            out.push_back(text[pos++]);
        } else {
            out.push_back(c);
        }
    }

    return false;
}

std::vector<std::map<std::string, std::string>> ParseEntityLump(const std::string& entities) {
    std::vector<std::map<std::string, std::string>> parsed;
    size_t pos = 0;

    while (pos < entities.size()) {
        SkipWhitespace(entities, pos);
        if (pos >= entities.size()) break;
        if (entities[pos] != '{') {
            ++pos;
            continue;
        }

        ++pos;
        std::map<std::string, std::string> values;
        while (pos < entities.size()) {
            SkipWhitespace(entities, pos);
            if (pos >= entities.size()) break;
            if (entities[pos] == '}') {
                ++pos;
                break;
            }

            std::string key;
            std::string value;
            if (!ReadQuotedToken(entities, pos, key) || !ReadQuotedToken(entities, pos, value)) {
                break;
            }
            values[key] = value;
        }

        if (!values.empty()) parsed.push_back(std::move(values));
    }

    return parsed;
}

bool ParseVec3(const std::string& text, float& x, float& y, float& z) {
    return std::sscanf(text.c_str(), "%f %f %f", &x, &y, &z) == 3;
}

std::array<float, 3> ConvertQ3Point(float qx, float qy, float qz, float scale) {
    return {qx * scale, qz * scale, -qy * scale};
}

nlohmann::json PointJson(const std::array<float, 3>& p) {
    return nlohmann::json::array({p[0], p[1], p[2]});
}

nlohmann::json ConvertModelBounds(const BspModel& model, float scale) {
    std::array<float, 3> mn{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    };
    std::array<float, 3> mx{
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()
    };

    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                const auto p = ConvertQ3Point(x ? model.maxs[0] : model.mins[0],
                                              y ? model.maxs[1] : model.mins[1],
                                              z ? model.maxs[2] : model.mins[2],
                                              scale);
                for (int axis = 0; axis < 3; ++axis) {
                    mn[axis] = std::min(mn[axis], p[axis]);
                    mx[axis] = std::max(mx[axis], p[axis]);
                }
            }
        }
    }

    return nlohmann::json{{"min", PointJson(mn)}, {"max", PointJson(mx)}};
}

}  // namespace

WorkflowBspParseSpawnStep::WorkflowBspParseSpawnStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspParseSpawnStep::GetPluginId() const {
    return "bsp.parse_spawn";
}

void WorkflowBspParseSpawnStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    auto bspDataPtr = context.Get<std::shared_ptr<std::vector<uint8_t>>>("bsp_raw_data", nullptr);
    if (!bspDataPtr) throw std::runtime_error("bsp.parse_spawn: bsp_raw_data not in context");

    auto bspConfig = context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    float scale = bspConfig.value("scale", 1.0f / 32.0f);

    const auto& bspData = *bspDataPtr;
    auto* lumps = reinterpret_cast<const BspLump*>(bspData.data() + sizeof(BspHeader));
    const auto& entLump = lumps[LUMP_ENTITIES];
    std::string entities(reinterpret_cast<const char*>(bspData.data() + entLump.offset), entLump.length);
    const auto parsedEntities = ParseEntityLump(entities);

    std::vector<BspModel> models;
    const auto& modelLump = lumps[LUMP_MODELS];
    if (modelLump.length >= 0 && modelLump.offset >= 0 &&
        static_cast<size_t>(modelLump.offset + modelLump.length) <= bspData.size()) {
        const auto* modelData = reinterpret_cast<const BspModel*>(bspData.data() + modelLump.offset);
        const size_t modelCount = static_cast<size_t>(modelLump.length) / sizeof(BspModel);
        models.assign(modelData, modelData + modelCount);
    }

    float spawnX = 0, spawnY = 5, spawnZ = 0;
    float spawnAngle = 0;
    nlohmann::json entityJson = nlohmann::json::array();
    std::unordered_map<std::string, std::array<float, 3>> targets;
    std::map<std::string, int> classCounts;
    int pickupCount = 0;
    int jumpPadCount = 0;
    int teleporterCount = 0;

    for (size_t i = 0; i < parsedEntities.size(); ++i) {
        const auto& values = parsedEntities[i];
        nlohmann::json ent = nlohmann::json::object();
        for (const auto& [key, value] : values) {
            ent[key] = value;
        }

        const std::string classname = ent.value("classname", std::string{});
        const int classIndex = classCounts[classname]++;
        ent["id"] = classname.empty()
            ? "ent_" + std::to_string(i)
            : classname + "_" + std::to_string(classIndex);

        auto originIt = values.find("origin");
        if (originIt != values.end()) {
            float ox = 0, oy = 0, oz = 0;
            if (ParseVec3(originIt->second, ox, oy, oz)) {
                const auto p = ConvertQ3Point(ox, oy, oz, scale);
                ent["position"] = PointJson(p);

                auto targetNameIt = values.find("targetname");
                if (targetNameIt != values.end()) {
                    targets[targetNameIt->second] = p;
                }

                if (classname == "info_player_deathmatch" && spawnY == 5.0f) {
                    spawnX = p[0];
                    spawnY = p[1] + 1.0f;
                    spawnZ = p[2];
                }
            }
        }

        auto angleIt = values.find("angle");
        if (classname == "info_player_deathmatch" && angleIt != values.end()) {
            try {
                spawnAngle = std::stof(angleIt->second);
            } catch (...) {
                spawnAngle = 0.0f;
            }
        }

        auto modelIt = values.find("model");
        if (modelIt != values.end() && modelIt->second.size() > 1 && modelIt->second[0] == '*') {
            const int modelIndex = std::atoi(modelIt->second.c_str() + 1);
            if (modelIndex >= 0 && static_cast<size_t>(modelIndex) < models.size()) {
                ent["model_index"] = modelIndex;
                ent["bounds"] = ConvertModelBounds(models[static_cast<size_t>(modelIndex)], scale);
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

        entityJson.push_back(std::move(ent));
    }

    for (auto& ent : entityJson) {
        const std::string target = ent.value("target", std::string{});
        if (!target.empty()) {
            auto targetIt = targets.find(target);
            if (targetIt != targets.end()) {
                ent["target_position"] = PointJson(targetIt->second);
            }
        }
    }

    context.Set("bsp.spawn", nlohmann::json{
        {"x", spawnX}, {"y", spawnY}, {"z", spawnZ}, {"angle", spawnAngle}
    });
    context.Set("bsp.entities", entityJson);
    context.Set("bsp.entity_counts", nlohmann::json{
        {"total", entityJson.size()},
        {"pickups", pickupCount},
        {"jump_pads", jumpPadCount},
        {"teleporters", teleporterCount}
    });

    if (logger_) {
        logger_->Info("bsp.parse_spawn: Spawn at (" + std::to_string(spawnX) + ", " +
                     std::to_string(spawnY) + ", " + std::to_string(spawnZ) +
                     ") angle=" + std::to_string(spawnAngle));
        logger_->Info("bsp.parse_spawn: Parsed " + std::to_string(entityJson.size()) +
                      " entities (" + std::to_string(pickupCount) + " pickups, " +
                      std::to_string(jumpPadCount) + " jump pads, " +
                      std::to_string(teleporterCount) + " teleporters)");
    }
}

}  // namespace sdl3cpp::services::impl
