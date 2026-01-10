#pragma once

#include "../interfaces/i_logger.hpp"
#include "../interfaces/i_probe_service.hpp"
#include "../interfaces/i_workflow_step_registry.hpp"
#include "../interfaces/workflow_definition.hpp"

namespace sdl3cpp::services::impl {

class WorkflowDefaultStepRegistrar {
public:
    WorkflowDefaultStepRegistrar(std::shared_ptr<ILogger> logger,
                                 std::shared_ptr<IProbeService> probeService);

    void RegisterUsedSteps(const WorkflowDefinition& workflow,
                           const std::shared_ptr<IWorkflowStepRegistry>& registry) const;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IProbeService> probeService_;
};

}  // namespace sdl3cpp::services::impl
