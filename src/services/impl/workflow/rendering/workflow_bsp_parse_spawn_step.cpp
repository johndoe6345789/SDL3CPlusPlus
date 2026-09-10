#include "services/interfaces/workflow/rendering/workflow_bsp_parse_spawn_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_entity_json_builder.hpp"
#include "services/interfaces/workflow/rendering/bsp_entity_lump_parser.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowBspParseSpawnStep::WorkflowBspParseSpawnStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspParseSpawnStep::GetPluginId() const {
    return "bsp.parse_spawn";
}

void WorkflowBspParseSpawnStep::Execute(const WorkflowStepDefinition&,
                                        WorkflowContext& context) {
    auto bspDataPtr =
        context.Get<std::shared_ptr<std::vector<uint8_t>>>("bsp_raw_data",
                                                            nullptr);
    if (!bspDataPtr) {
        throw std::runtime_error("bsp.parse_spawn: bsp_raw_data not in context");
    }

    const auto bspConfig =
        context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    const float scale = bspConfig.value("scale", 1.0f / 32.0f);

    const auto& bspData = *bspDataPtr;
    const auto* lumps =
        reinterpret_cast<const BspLump*>(bspData.data() + sizeof(BspHeader));
    const auto& entLump = lumps[LUMP_ENTITIES];
    const std::string entitiesText(
        reinterpret_cast<const char*>(bspData.data() + entLump.offset),
        entLump.length);

    const BspEntitiesResult result = BuildBspEntitiesJson(
        ParseBspEntityLump(entitiesText),
        ReadBspModels(bspData, lumps[LUMP_MODELS]), scale);

    context.Set("bsp.spawn", result.spawn);
    context.Set("bsp.entities", result.entities);
    context.Set("bsp.entity_counts",
               nlohmann::json{{"total", result.entities.size()},
                              {"pickups", result.pickupCount},
                              {"jump_pads", result.jumpPadCount},
                              {"teleporters", result.teleporterCount}});

    if (logger_) {
        logger_->Info("bsp.parse_spawn: Spawn at (" +
                      std::to_string(result.spawn.value("x", 0.0f)) + ", " +
                      std::to_string(result.spawn.value("y", 0.0f)) + ", " +
                      std::to_string(result.spawn.value("z", 0.0f)) +
                      ") angle=" +
                      std::to_string(result.spawn.value("angle", 0.0f)));
        logger_->Info("bsp.parse_spawn: Parsed " +
                      std::to_string(result.entities.size()) + " entities (" +
                      std::to_string(result.pickupCount) + " pickups, " +
                      std::to_string(result.jumpPadCount) + " jump pads, " +
                      std::to_string(result.teleporterCount) +
                      " teleporters)");
    }
}

}  // namespace sdl3cpp::services::impl
