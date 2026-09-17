#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_graphics_service.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Generates a simple built-in constant color shader.
 *
 * This shader bypasses MaterialX and generates raw GLSL code for a simple
 * constant-color shader suitable for demos and testing.
 */
class WorkflowShaderBuiltinConstantColorStep : public IWorkflowStep {
public:
    explicit WorkflowShaderBuiltinConstantColorStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<IGraphicsService> graphicsService);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IGraphicsService> graphicsService_;
};

}  // namespace sdl3cpp::services::impl
