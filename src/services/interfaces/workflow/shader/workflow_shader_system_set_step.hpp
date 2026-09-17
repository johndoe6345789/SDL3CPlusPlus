#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services {
class IShaderSystemRegistry;
}

namespace sdl3cpp::services::impl {

class WorkflowShaderSystemSetStep : public IWorkflowStep {
public:
    explicit WorkflowShaderSystemSetStep(std::shared_ptr<ILogger> logger,
                                        std::shared_ptr<IShaderSystemRegistry> shaderRegistry);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IShaderSystemRegistry> shaderRegistry_;
};

}  // namespace sdl3cpp::services::impl
