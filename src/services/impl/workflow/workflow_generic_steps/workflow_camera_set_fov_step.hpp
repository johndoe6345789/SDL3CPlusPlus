#pragma once

#include "../../interfaces/camera_types.hpp"
#include "../../interfaces/i_logger.hpp"
#include "../../interfaces/i_workflow_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowCameraSetFovStep final : public IWorkflowStep {
public:
    explicit WorkflowCameraSetFovStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
