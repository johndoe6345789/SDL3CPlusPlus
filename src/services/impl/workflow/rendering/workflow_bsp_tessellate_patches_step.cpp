#include "services/interfaces/workflow/rendering/workflow_bsp_tessellate_patches_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_geometry_lumps.hpp"
#include "services/interfaces/workflow/rendering/bsp_face_visibility.hpp"
#include "services/interfaces/workflow/rendering/bsp_patch_face_builder.hpp"
#include "services/interfaces/workflow/rendering/bsp_texture_groups_context.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {
namespace {

int ReadPatchTessLevel(const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    const auto* p       = params.FindParameter(step, "patch_tess_level");
    const bool isNumber = p && p->type == WorkflowParameterValue::Type::Number;
    return isNumber ? static_cast<int>(p->numberValue) : 4;
}

}  // namespace

WorkflowBspTessellatePatchesStep::WorkflowBspTessellatePatchesStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspTessellatePatchesStep::GetPluginId() const {
    return "bsp.tessellate_patches";
}

void WorkflowBspTessellatePatchesStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    auto bspDataPtr = context.Get<std::shared_ptr<std::vector<uint8_t>>>(
        "bsp_raw_data", nullptr);
    if (!bspDataPtr) {
        throw std::runtime_error(
            "bsp.tessellate_patches: bsp_raw_data not in context");
    }

    const auto bspConfig =
        context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    const float scale        = bspConfig.value("scale", 1.0f / 32.0f);
    const int gridSize       = context.Get<int>("bsp_grid_size", 1);
    const int numLightmaps   = context.Get<int>("bsp_num_lightmaps", 0);
    const int patchTessLevel = ReadPatchTessLevel(step);

    const BspGeometryLumps lumps = ReadBspGeometryLumps(*bspDataPtr);
    auto groups                  = GetOrCreateBspTextureGroups(context);

    int tessellatedPatches = 0;
    for (int f = 0; f < lumps.numFaces; ++f) {
        const auto& face = lumps.faces[f];
        if (face.type != 2) {
            continue;
        }
        if (face.texture >= 0 && face.texture < lumps.numTextures &&
            !IsBspFaceTextureVisible(lumps.textures[face.texture].name)) {
            continue;
        }

        if (AppendBspPatchFace(lumps, face, scale, gridSize, numLightmaps,
                               patchTessLevel, (*groups)[face.texture])) {
            ++tessellatedPatches;
        }
    }

    if (logger_) {
        logger_->Info(
            "bsp.tessellate_patches: " + std::to_string(tessellatedPatches) +
            " patches tessellated");
    }
}

}  // namespace sdl3cpp::services::impl
