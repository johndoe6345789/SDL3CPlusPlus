#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// graphics.gpu.pipeline.create's parameters, all with their defaults.
struct GpuPipelineCreateParams {
    std::string vertexShaderKey   = "vertex_shader";
    std::string fragmentShaderKey = "fragment_shader";
    std::string vertexFormat      = "position_color";
    std::string pipelineKey       = "gpu_pipeline";
    bool depthWrite               = true;
    bool depthTest                = true;
    std::string cullMode          = "back";
    float depthBias               = 0.0f;
    float depthBiasSlope          = 0.0f;
    int numColorTargets           = 1;
    std::string depthFormat       = "d32_float";
    bool releaseShaders           = true;
    std::string colorFormat       = "swapchain";
    bool hasDepth                 = true;
    bool alphaBlend               = false;
};

/// Reads GpuPipelineCreateParams from the step's parameters, applying the
/// struct's defaults for anything absent or of the wrong type.
GpuPipelineCreateParams ReadGpuPipelineCreateParams(
    const WorkflowStepDefinition& step);

}  // namespace sdl3cpp::services::impl
