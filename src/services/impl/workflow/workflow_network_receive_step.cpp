#include "services/interfaces/workflow/workflow_network_receive_step.hpp"
#include "services/interfaces/workflow/network_receive_helpers.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowNetworkReceiveStep::WorkflowNetworkReceiveStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowNetworkReceiveStep", "Constructor", "Entry");
    }
}

std::string WorkflowNetworkReceiveStep::GetPluginId() const {
    return "network.receive";
}

void WorkflowNetworkReceiveStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowNetworkReceiveStep", "Execute", "Entry");
    }

    const NetworkReceiveParams params = ReadNetworkReceiveParams(step);
    if (logger_) {
        logger_->Trace("WorkflowNetworkReceiveStep", "Execute", "connection_id",
                       params.connectionId);
        logger_->Trace("WorkflowNetworkReceiveStep", "Execute", "timeout",
                       std::to_string(params.timeout));
    }

    const NetworkReceiveResult result =
        TryDequeueMessage(messageQueues_, params, logger_);

    const NetworkReceiveOutputKeys keys = ResolveNetworkReceiveOutputKeys(step);
    context.Set(keys.receivedKey, result.received);
    context.Set(keys.payloadKey, result.payload);
    context.Set(keys.bytesReceivedKey,
                static_cast<double>(result.bytesReceived));

    if (logger_) {
        logger_->Trace("WorkflowNetworkReceiveStep", "Execute", "received",
                       result.received ? "true" : "false");
        logger_->Trace("WorkflowNetworkReceiveStep", "Execute",
                       "bytes_received", std::to_string(result.bytesReceived));
    }
}

}  // namespace sdl3cpp::services::impl
