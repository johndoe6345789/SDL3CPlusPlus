#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include <memory>

struct SDL_Window;

namespace sdl3cpp::services::impl {

class WorkflowSdlWindowCreateStep final : public IWorkflowStep {
public:
    explicit WorkflowSdlWindowCreateStep(std::shared_ptr<ILogger> logger);
    ~WorkflowSdlWindowCreateStep() override = default;

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
