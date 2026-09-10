#include "services/interfaces/workflow/rendering/viewmodel_draw.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

namespace {

std::string GetStrParam(const WorkflowStepParameterResolver& params,
                        const WorkflowStepDefinition& step, const char* name,
                        const std::string& def) {
    const auto* p = params.FindParameter(step, name);
    return (p && p->type == WorkflowParameterValue::Type::String)
               ? p->stringValue
               : def;
}

float GetNumParam(const WorkflowStepParameterResolver& params,
                  const WorkflowStepDefinition& step, const char* name,
                  float def) {
    const auto* p = params.FindParameter(step, name);
    return (p && p->type == WorkflowParameterValue::Type::Number)
               ? static_cast<float>(p->numberValue)
               : def;
}

}  // namespace

ViewmodelDrawParams ReadViewmodelDrawParams(
    const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    ViewmodelDrawParams out;
    out.meshName  = GetStrParam(params, step, "mesh", out.meshName);
    out.texName   = GetStrParam(params, step, "texture", out.texName);
    out.offsetX   = GetNumParam(params, step, "offset_x", out.offsetX);
    out.offsetY   = GetNumParam(params, step, "offset_y", out.offsetY);
    out.offsetZ   = GetNumParam(params, step, "offset_z", out.offsetZ);
    out.scale     = GetNumParam(params, step, "scale", out.scale);
    out.rotX      = GetNumParam(params, step, "rot_x", out.rotX);
    out.rotY      = GetNumParam(params, step, "rot_y", out.rotY);
    out.rotZ      = GetNumParam(params, step, "rot_z", out.rotZ);
    out.roughness = GetNumParam(params, step, "roughness", out.roughness);
    out.metallic  = GetNumParam(params, step, "metallic", out.metallic);
    return out;
}

}  // namespace sdl3cpp::services::impl
