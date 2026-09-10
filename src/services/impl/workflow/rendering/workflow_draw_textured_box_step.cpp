#include "services/interfaces/workflow/rendering/workflow_draw_textured_box_step.hpp"
#include "services/interfaces/workflow/rendering/box_face_geometry.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <string>

namespace sdl3cpp::services::impl {

WorkflowDrawTexturedBoxStep::WorkflowDrawTexturedBoxStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDrawTexturedBoxStep::GetPluginId() const {
    return "draw.textured_box";
}

void WorkflowDrawTexturedBoxStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    WorkflowStepParameterResolver params;
    auto getStr = [&](const char* name, const std::string& def) {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::String)
                   ? p->stringValue
                   : def;
    };
    auto getNum = [&](const char* name, float def) {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<float>(p->numberValue)
                   : def;
    };

    DrawTexturedBoxParams box;
    box.pos       = glm::vec3(getNum("pos_x", 0.0f), getNum("pos_y", 0.0f),
                              getNum("pos_z", 0.0f));
    box.size      = glm::vec3(getNum("size_x", 1.0f), getNum("size_y", 1.0f),
                              getNum("size_z", 1.0f));
    box.uvDensity = getNum("uv_density", 1.0f);
    box.roughness = getNum("roughness", 0.8f);
    box.metallic  = getNum("metallic", 0.0f);
    box.texture   = getStr("texture", "walls_texture");
    box.body      = getStr("body", "");

    DrawTexturedBox(context, logger_.get(), box);
}

}  // namespace sdl3cpp::services::impl
