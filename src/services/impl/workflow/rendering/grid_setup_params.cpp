#include "services/interfaces/workflow/rendering/grid_setup_params.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <type_traits>

namespace sdl3cpp::services::impl {

GridSetupParams ReadGridSetupParams(const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver paramResolver;
    GridSetupParams out;
    auto readParam = [&](const char* name, auto& value) {
        if (const auto* p = paramResolver.FindParameter(step, name)) {
            if (p->type == WorkflowParameterValue::Type::Number) {
                value = static_cast<std::remove_reference_t<decltype(value)>>(
                    p->numberValue);
            }
        }
    };
    readParam("grid_width", out.gridWidth);
    readParam("grid_height", out.gridHeight);
    readParam("grid_spacing", out.gridSpacing);
    readParam("grid_start_x", out.gridStartX);
    readParam("grid_start_y", out.gridStartY);
    readParam("rotation_offset_x", out.rotationOffsetX);
    readParam("rotation_offset_y", out.rotationOffsetY);
    readParam("num_frames", out.numFrames);
    readParam("background_color_r", out.bgColorR);
    readParam("background_color_g", out.bgColorG);
    readParam("background_color_b", out.bgColorB);
    return out;
}

nlohmann::json BuildGridConfigJson(const GridSetupParams& params) {
    return nlohmann::json{
        {"grid_width", params.gridWidth},
        {"grid_height", params.gridHeight},
        {"grid_spacing", params.gridSpacing},
        {"grid_start_x", params.gridStartX},
        {"grid_start_y", params.gridStartY},
        {"rotation_offset_x", params.rotationOffsetX},
        {"rotation_offset_y", params.rotationOffsetY},
        {"background_color_r", params.bgColorR},
        {"background_color_g", params.bgColorG},
        {"background_color_b", params.bgColorB},
        {"num_frames", params.numFrames},
    };
}

}  // namespace sdl3cpp::services::impl
