#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include <memory>
#include <filesystem>

namespace sdl3cpp::services::impl {

class WorkflowAppInitStep final : public IWorkflowStep {
public:
    explicit WorkflowAppInitStep(std::shared_ptr<ILogger> logger);
    ~WorkflowAppInitStep() override = default;

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
