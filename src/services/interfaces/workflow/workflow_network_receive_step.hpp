#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>
#include <queue>
#include <vector>
#include <map>

namespace sdl3cpp::services::impl {

/**
 * @brief Network receive workflow step
 * Receives data from network connection
 * Parameters: connection_id (string), timeout (number, optional)
 * Output: received (bool), payload (string), bytes_received (number)
 */
class WorkflowNetworkReceiveStep : public IWorkflowStep {
public:
    explicit WorkflowNetworkReceiveStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::map<std::string, std::queue<std::string>> messageQueues_;
};

}  // namespace sdl3cpp::services::impl
