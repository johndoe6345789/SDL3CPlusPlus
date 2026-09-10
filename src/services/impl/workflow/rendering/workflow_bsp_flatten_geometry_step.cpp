#include "services/interfaces/workflow/rendering/workflow_bsp_flatten_geometry_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_geometry_lumps.hpp"
#include "services/interfaces/workflow/rendering/bsp_texture_group_flattener.hpp"
#include "services/interfaces/workflow/rendering/bsp_texture_groups_context.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowBspFlattenGeometryStep::WorkflowBspFlattenGeometryStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspFlattenGeometryStep::GetPluginId() const {
    return "bsp.flatten_geometry";
}

void WorkflowBspFlattenGeometryStep::Execute(const WorkflowStepDefinition&,
                                             WorkflowContext& context) {
    auto groups = GetBspTextureGroups(context);
    if (!groups) {
        throw std::runtime_error(
            "bsp.flatten_geometry: bsp_texture_groups not in context "
            "(run bsp.tessellate_patches and bsp.build_polygons first)");
    }

    // Only used to resolve each group's texture name; empty view if the raw
    // BSP buffer is no longer in the context (names are then left blank).
    BspGeometryLumps lumps;
    if (auto bspDataPtr = context.Get<std::shared_ptr<std::vector<uint8_t>>>(
            "bsp_raw_data", nullptr)) {
        lumps = ReadBspGeometryLumps(*bspDataPtr);
    }

    const auto bspConfig =
        context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    const std::string mapName =
        bspConfig.value("map_name", std::string("q3dm17"));

    const FlattenedBspGeometry flattened = FlattenBspTextureGroups(
        *groups, mapName, lumps.numTextures, lumps.textures);

    if (flattened.vertices->empty() || flattened.indices->empty()) {
        throw std::runtime_error(
            "bsp.flatten_geometry: No renderable geometry found");
    }

    context.Set("bsp_all_vertices", flattened.vertices);
    context.Set("bsp_all_indices", flattened.indices);
    context.Set("bsp_used_textures", flattened.usedTextures);
    context.Set("map.nodes", flattened.mapNodes);

    if (logger_) {
        logger_->Info(
            "bsp.flatten_geometry: " +
            std::to_string(flattened.vertices->size()) + " vertices, " +
            std::to_string(flattened.indices->size()) + " indices, " +
            std::to_string(flattened.mapNodes.size()) + " texture groups");
    }
}

}  // namespace sdl3cpp::services::impl
