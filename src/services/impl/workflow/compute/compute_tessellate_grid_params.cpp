#include "services/interfaces/workflow/compute/compute_tessellate_grid.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {
namespace {

float NumberParameter(const WorkflowStepParameterResolver& params,
                      const WorkflowStepDefinition& step, const char* key,
                      float fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isNumber = p && p->type == WorkflowParameterValue::Type::Number;
    return isNumber ? static_cast<float>(p->numberValue) : fallback;
}

std::string StringParameter(const WorkflowStepParameterResolver& params,
                            const WorkflowStepDefinition& step, const char* key,
                            const std::string& fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isString = p && p->type == WorkflowParameterValue::Type::String;
    return isString ? p->stringValue : fallback;
}

}  // namespace

TessellationGridParams ReadTessellationGridParams(
    const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    TessellationGridParams p;
    p.width                = NumberParameter(params, step, "width", p.width);
    p.depth                = NumberParameter(params, step, "depth", p.depth);
    p.subdivisions         = static_cast<int>(NumberParameter(
        params, step, "subdivisions", static_cast<float>(p.subdivisions)));
    p.displacementStrength = NumberParameter(
        params, step, "displacement_strength", p.displacementStrength);
    p.uvScaleX = NumberParameter(params, step, "uv_scale_x", p.uvScaleX);
    p.uvScaleY = NumberParameter(params, step, "uv_scale_y", p.uvScaleY);
    p.name     = StringParameter(params, step, "name", p.name);
    return p;
}

}  // namespace sdl3cpp::services::impl
