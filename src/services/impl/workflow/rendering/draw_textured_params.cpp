#include "services/interfaces/workflow/rendering/draw_textured_params.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

DrawTexturedParams ReadDrawTexturedParams(const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;

    auto getStr = [&](const char* name, const std::string& def) {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::String)
                   ? p->stringValue
                   : def;
    };
    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<float>(p->numberValue)
                   : def;
    };

    DrawTexturedParams result;
    result.meshName    = getStr("mesh", "plane");
    result.textureName = getStr("texture", "texture");
    result.facing      = getStr("facing", "");
    result.posX        = getNum("pos_x", 0.0f);
    result.posY        = getNum("pos_y", 0.0f);
    result.posZ        = getNum("pos_z", 0.0f);
    result.rotX        = getNum("rot_x", 0.0f);
    result.rotY        = getNum("rot_y", 0.0f);
    result.rotZ        = getNum("rot_z", 0.0f);
    result.scale       = getNum("scale", 1.0f);
    result.roughness   = getNum("roughness", 0.8f);
    result.metallic    = getNum("metallic", 0.0f);
    return result;
}

}  // namespace sdl3cpp::services::impl
