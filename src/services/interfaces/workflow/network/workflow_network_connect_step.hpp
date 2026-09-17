#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>
#include <queue>

namespace sdl3cpp::services::impl {

/**
 * @brief Network connection workflow step
 * Establishes connection to a server or peer
 * Parameters: host (string), port (number), timeout (number, optional)
 * Output: connection_id (string), connected (bool)
 */
class WorkflowNetworkConnectStep : public IWorkflowStep {
public:
    explicit WorkflowNetworkConnectStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    int nextConnectionId_ = 0;
};

}  // namespace sdl3cpp::services::impl
