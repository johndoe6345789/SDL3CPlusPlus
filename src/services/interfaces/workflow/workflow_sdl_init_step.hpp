#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowSdlInitStep final : public IWorkflowStep {
public:
    explicit WorkflowSdlInitStep(std::shared_ptr<ILogger> logger);
    ~WorkflowSdlInitStep() override = default;

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
