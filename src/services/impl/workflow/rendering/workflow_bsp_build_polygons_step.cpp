#include "services/interfaces/workflow/rendering/workflow_bsp_build_polygons_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_geometry_lumps.hpp"
#include "services/interfaces/workflow/rendering/bsp_face_visibility.hpp"
#include "services/interfaces/workflow/rendering/bsp_polygon_face_builder.hpp"
#include "services/interfaces/workflow/rendering/bsp_texture_groups_context.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowBspBuildPolygonsStep::WorkflowBspBuildPolygonsStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspBuildPolygonsStep::GetPluginId() const {
    return "bsp.build_polygons";
}

void WorkflowBspBuildPolygonsStep::Execute(const WorkflowStepDefinition&,
                                           WorkflowContext& context) {
    auto bspDataPtr = context.Get<std::shared_ptr<std::vector<uint8_t>>>(
        "bsp_raw_data", nullptr);
    if (!bspDataPtr) {
        throw std::runtime_error(
            "bsp.build_polygons: bsp_raw_data not in context");
    }

    auto groups = GetBspTextureGroups(context);
    if (!groups) {
        throw std::runtime_error(
            "bsp.build_polygons: bsp_texture_groups not in context "
            "(run bsp.tessellate_patches first)");
    }

    const auto bspConfig =
        context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    const float scale      = bspConfig.value("scale", 1.0f / 32.0f);
    const int gridSize     = context.Get<int>("bsp_grid_size", 1);
    const int numLightmaps = context.Get<int>("bsp_num_lightmaps", 0);

    const BspGeometryLumps lumps = ReadBspGeometryLumps(*bspDataPtr);

    int polygonFaces = 0;
    for (int f = 0; f < lumps.numFaces; ++f) {
        const auto& face = lumps.faces[f];
        if (face.type != 1 && face.type != 3) {
            continue;
        }
        if (face.texture >= 0 && face.texture < lumps.numTextures &&
            !IsBspFaceTextureVisible(lumps.textures[face.texture].name)) {
            continue;
        }

        AppendBspPolygonFace(lumps, face, scale, gridSize, numLightmaps,
                             (*groups)[face.texture]);
        ++polygonFaces;
    }

    if (logger_) {
        logger_->Info("bsp.build_polygons: " + std::to_string(polygonFaces) +
                      " polygon/mesh faces");
    }
}

}  // namespace sdl3cpp::services::impl
