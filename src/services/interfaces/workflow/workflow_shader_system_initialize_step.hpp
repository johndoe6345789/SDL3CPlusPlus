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

class WorkflowShaderSystemInitializeStep : public IWorkflowStep {
public:
    explicit WorkflowShaderSystemInitializeStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<IShaderSystemRegistry> shaderRegistry,
        std::shared_ptr<IGraphicsService> graphicsService,
        std::shared_ptr<IWorkflowExecutor> workflowExecutor = nullptr);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    // Phase 1: Set active shader system
    void ExecuteSystemSet(const WorkflowStepDefinition& step, WorkflowContext& context);

    // Phase 2: Load glTF models
    void ExecuteGltfLoad(const WorkflowStepDefinition& step, WorkflowContext& context);

    // Phase 3: Compile shaders
    void ExecuteCompile(const WorkflowStepDefinition& step, WorkflowContext& context);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IShaderSystemRegistry> shaderRegistry_;
    std::shared_ptr<IGraphicsService> graphicsService_;
    std::shared_ptr<IWorkflowExecutor> workflowExecutor_;
};

}  // namespace sdl3cpp::services::impl
