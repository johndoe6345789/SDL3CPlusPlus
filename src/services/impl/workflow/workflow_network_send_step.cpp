#include "services/interfaces/workflow/workflow_network_send_step.hpp"
#include "services/interfaces/workflow/network_send_simulation.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowNetworkSendStep::WorkflowNetworkSendStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowNetworkSendStep", "Constructor", "Entry");
    }
}

std::string WorkflowNetworkSendStep::GetPluginId() const {
    return "network.send";
}

void WorkflowNetworkSendStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowNetworkSendStep", "Execute", "Entry");
    }

    const NetworkSendRequest request = ReadNetworkSendRequest(step);

    if (logger_) {
        logger_->Trace("WorkflowNetworkSendStep", "Execute",
                       "connection_id", request.connectionId);
        logger_->Trace("WorkflowNetworkSendStep", "Execute", "payload_size",
                       std::to_string(request.payload.size()));
        logger_->Trace("WorkflowNetworkSendStep", "Execute", "priority",
                       std::to_string(request.priority));
    }

    const NetworkSendOutcome outcome = SimulateNetworkSend(request, logger_);
    totalBytesSent_ += outcome.bytesSent;

    // Get output keys from step definition or use defaults
    WorkflowStepIoResolver resolver;
    std::string sentKey = "network.sent";
    std::string bytesSentKey = "network.bytes_sent";

    try {
        sentKey = resolver.GetRequiredOutputKey(step, "sent");
    } catch (...) {
        // Use default
    }

    try {
        bytesSentKey = resolver.GetRequiredOutputKey(step, "bytes_sent");
    } catch (...) {
        // Use default
    }

    context.Set(sentKey, outcome.sent);
    context.Set(bytesSentKey, static_cast<double>(outcome.bytesSent));

    if (logger_) {
        logger_->Trace("WorkflowNetworkSendStep", "Execute", "sent",
                       outcome.sent ? "true" : "false");
        logger_->Trace("WorkflowNetworkSendStep", "Execute", "bytes_sent",
                       std::to_string(outcome.bytesSent));
    }
}

}  // namespace sdl3cpp::services::impl
