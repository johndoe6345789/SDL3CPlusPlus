#pragma once

#include "services/interfaces/i_shader_system_registry.hpp"
#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow_definition.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Workflow step to explicitly compile shaders from the active shader system.
 *
 * This step triggers compilation of shaders using the active shader system
 * (e.g., MaterialX). It should be called after shader system setup (e.g., after
 * shader.system.set steps).
 *
 * This makes shader compilation explicit and controllable from workflows,
 * replacing the implicit compilation that happens during first render frame.
 *
 * Inputs: (from workflow context)
 *   - None required
 *
 * Outputs: (to workflow context)
 *   - "shader.compiled_count" (int) - Number of shaders compiled
 *   - "shader.keys" (vector<string>) - Array of available shader keys
 *   - "shader.compile_status" (string) - "success" or "failed"
 *   - "shader.error_message" (string) - Error details if compilation failed
 */
class WorkflowShaderCompileStep : public IWorkflowStep {
public:
    /**
     * @brief Construct shader compile step with logger and services
     *
     * @param logger Logger service for diagnostics
     * @param shaderRegistry Shader system registry (source of active system)
     * @param graphicsService Graphics service to load compiled shaders to GPU
     */
    WorkflowShaderCompileStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<IShaderSystemRegistry> shaderRegistry,
        std::shared_ptr<IGraphicsService> graphicsService);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IShaderSystemRegistry> shaderRegistry_;
    std::shared_ptr<IGraphicsService> graphicsService_;
};

}  // namespace sdl3cpp::services::impl
