#include "services/interfaces/workflow/graphics/shader_compile_params.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

ShaderCompileParams ReadShaderCompileParams(const WorkflowStepDefinition& step,
                                            const WorkflowContext& context) {
    WorkflowStepParameterResolver params;

    auto getStr = [&](const char* name, const std::string& def) {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::String)
                   ? p->stringValue
                   : def;
    };
    auto getInt = [&](const char* name, int def) -> int {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<int>(p->numberValue)
                   : def;
    };

    ShaderCompileParams result;
    result.shaderPath        = getStr("shader_path", "");
    result.stage             = getStr("stage", "vertex");
    result.numUniformBuffers = getInt("num_uniform_buffers", 0);
    result.numSamplers       = getInt("num_samplers", 0);
    result.outputKey         = getStr("output_key", "compiled_shader");

    // Fallback: resolve shader_path from inputs (for JSON workflow usage).
    if (result.shaderPath.empty()) {
        auto it = step.inputs.find("shader_path");
        if (it != step.inputs.end() && !it->second.empty()) {
            const auto* pathPtr = context.TryGet<std::string>(it->second);
            if (pathPtr) result.shaderPath = *pathPtr;
        }
    }
    return result;
}

}  // namespace sdl3cpp::services::impl
