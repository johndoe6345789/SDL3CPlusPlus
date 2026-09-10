#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services {
class IShaderSystemRegistry;
class IGraphicsService;
class IWorkflowExecutor;
}

namespace sdl3cpp::services::impl {

/**
 * @brief Phase 3 of shader system initialization: compiles and uploads
 * shaders.
 *
 * Final step in the `shader.system.initialize` chain (see that step's
 * doc comment). On success also marks the whole chain complete via
 * shader.init_status, matching the original monolithic step's
 * try/catch scope, which covered all three phases.
 *
 * Context Output:
 *   - shader.compile_status (std::string)
 *   - shader.compiled_count (double)
 *   - shader.init_status (std::string) = "complete" or "error"
 *   - shader.error_message (std::string), only set on error
 */
class WorkflowShaderSystemCompileStep : public IWorkflowStep {
public:
    explicit WorkflowShaderSystemCompileStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<IShaderSystemRegistry> shaderRegistry,
        std::shared_ptr<IGraphicsService> graphicsService,
        std::shared_ptr<IWorkflowExecutor> workflowExecutor = nullptr);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IShaderSystemRegistry> shaderRegistry_;
    std::shared_ptr<IGraphicsService> graphicsService_;
    std::shared_ptr<IWorkflowExecutor> workflowExecutor_;
};

}  // namespace sdl3cpp::services::impl
