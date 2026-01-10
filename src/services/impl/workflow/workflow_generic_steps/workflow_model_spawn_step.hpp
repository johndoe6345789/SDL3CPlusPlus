#pragma once

#include "../../interfaces/i_logger.hpp"
#include "../../interfaces/i_workflow_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowModelSpawnStep final : public IWorkflowStep {
public:
    explicit WorkflowModelSpawnStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
