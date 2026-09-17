#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

class WorkflowCmdlineArgsStep final : public IWorkflowStep {
public:
    explicit WorkflowCmdlineArgsStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
