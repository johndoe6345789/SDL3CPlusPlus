#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowGeometryCreateCubeStep final : public IWorkflowStep {
public:
    explicit WorkflowGeometryCreateCubeStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
