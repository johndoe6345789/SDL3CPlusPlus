#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// graphics.gpu.shader.compile's resolved parameters, each with the
/// original defaults; `shaderPath` falls back to a wired context input
/// (`step.inputs["shader_path"]`) when the parameter itself is empty.
struct ShaderCompileParams {
    std::string shaderPath;
    std::string stage     = "vertex";
    int numUniformBuffers = 0;
    int numSamplers       = 0;
    std::string outputKey = "compiled_shader";
};

ShaderCompileParams ReadShaderCompileParams(const WorkflowStepDefinition& step,
                                            const WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
