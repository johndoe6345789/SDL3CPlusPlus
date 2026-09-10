#include "services/interfaces/workflow/graphics/gpu_pipeline_params.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {
namespace {

std::string StrParam(const WorkflowStepParameterResolver& params,
                     const WorkflowStepDefinition& step, const char* key,
                     const std::string& fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isString = p && p->type == WorkflowParameterValue::Type::String;
    return isString ? p->stringValue : fallback;
}

float NumParam(const WorkflowStepParameterResolver& params,
               const WorkflowStepDefinition& step, const char* key,
               float fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isNumber = p && p->type == WorkflowParameterValue::Type::Number;
    return isNumber ? static_cast<float>(p->numberValue) : fallback;
}

bool BoolParam(const WorkflowStepParameterResolver& params,
               const WorkflowStepDefinition& step, const char* key,
               bool fallback) {
    return static_cast<int>(NumParam(params, step, key, fallback ? 1 : 0)) != 0;
}

}  // namespace

GpuPipelineCreateParams ReadGpuPipelineCreateParams(
    const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    GpuPipelineCreateParams p;

    p.vertexShaderKey =
        StrParam(params, step, "vertex_shader_key", p.vertexShaderKey);
    p.fragmentShaderKey =
        StrParam(params, step, "fragment_shader_key", p.fragmentShaderKey);
    p.vertexFormat = StrParam(params, step, "vertex_format", p.vertexFormat);
    p.pipelineKey  = StrParam(params, step, "pipeline_key", p.pipelineKey);
    p.depthWrite   = BoolParam(params, step, "depth_write", p.depthWrite);
    p.depthTest    = BoolParam(params, step, "depth_test", p.depthTest);
    p.cullMode     = StrParam(params, step, "cull_mode", p.cullMode);
    p.depthBias    = NumParam(params, step, "depth_bias", p.depthBias);
    p.depthBiasSlope =
        NumParam(params, step, "depth_bias_slope", p.depthBiasSlope);
    p.numColorTargets = static_cast<int>(
        NumParam(params, step, "num_color_targets", p.numColorTargets));
    p.depthFormat = StrParam(params, step, "depth_format", p.depthFormat);
    p.releaseShaders =
        BoolParam(params, step, "release_shaders", p.releaseShaders);
    p.colorFormat = StrParam(params, step, "color_format", p.colorFormat);
    p.hasDepth    = BoolParam(params, step, "has_depth", p.hasDepth);
    p.alphaBlend  = BoolParam(params, step, "alpha_blend", p.alphaBlend);
    return p;
}

}  // namespace sdl3cpp::services::impl
