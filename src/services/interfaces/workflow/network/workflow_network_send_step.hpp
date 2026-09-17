#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Network send workflow step
 * Sends data over network connection
 * Parameters: connection_id (string), payload (string), priority (number, optional)
 * Output: sent (bool), bytes_sent (number)
 */
class WorkflowNetworkSendStep : public IWorkflowStep {
public:
    explicit WorkflowNetworkSendStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    uint64_t totalBytesSent_ = 0;
};

}  // namespace sdl3cpp::services::impl
