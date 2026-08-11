#include "services/interfaces/workflow/workflow_network_receive_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowNetworkReceiveStep::WorkflowNetworkReceiveStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowNetworkReceiveStep", "Constructor", "Entry");
    }
}

std::string WorkflowNetworkReceiveStep::GetPluginId() const {
    return "network.receive";
}

void WorkflowNetworkReceiveStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowNetworkReceiveStep", "Execute", "Entry");
    }

    // Get connection ID parameter (required)
    std::string connectionId;
    if (auto connIdParam = step.parameters.find("connection_id"); connIdParam != step.parameters.end() &&
        connIdParam->second.type == WorkflowParameterValue::Type::String) {
        connectionId = connIdParam->second.stringValue;
    } else {
        throw std::runtime_error("Workflow network.receive missing connection_id parameter");
    }

    // Get timeout parameter (optional, default: 1000 ms)
    int timeout = 1000;
    if (auto timeoutParam = step.parameters.find("timeout"); timeoutParam != step.parameters.end() &&
        timeoutParam->second.type == WorkflowParameterValue::Type::Number) {
        timeout = static_cast<int>(timeoutParam->second.numberValue);
    }

    if (logger_) {
        logger_->Trace("WorkflowNetworkReceiveStep", "Execute", "connection_id", connectionId);
        logger_->Trace("WorkflowNetworkReceiveStep", "Execute", "timeout", std::to_string(timeout));
    }

    // Validate connection ID
    bool received = false;
    std::string payload;
    uint64_t bytesReceived = 0;

    if (connectionId.empty()) {
        if (logger_) {
            logger_->Trace("WorkflowNetworkReceiveStep", "Execute", "Error", "Empty connection_id");
        }
    } else if (timeout < 0) {
        if (logger_) {
            logger_->Trace("WorkflowNetworkReceiveStep", "Execute", "Error", "Negative timeout");
        }
    } else {
        // Check for queued messages
        auto it = messageQueues_.find(connectionId);
        if (it != messageQueues_.end() && !it->second.empty()) {
            payload = it->second.front();
            it->second.pop();
            received = true;
            bytesReceived = payload.size();
            if (logger_) {
                logger_->Trace("WorkflowNetworkReceiveStep", "Execute", "Dequeued message", payload);
            }
        }
    }

    // Get output keys from step definition or use defaults
    WorkflowStepIoResolver resolver;
    std::string receivedKey = "network.received";
    std::string payloadKey = "network.payload";
    std::string bytesReceivedKey = "network.bytes_received";

    try {
        receivedKey = resolver.GetRequiredOutputKey(step, "received");
    } catch (...) {
        // Use default
    }

    try {
        payloadKey = resolver.GetRequiredOutputKey(step, "payload");
    } catch (...) {
        // Use default
    }

    try {
        bytesReceivedKey = resolver.GetRequiredOutputKey(step, "bytes_received");
    } catch (...) {
        // Use default
    }

    context.Set(receivedKey, received);
    context.Set(payloadKey, payload);
    context.Set(bytesReceivedKey, static_cast<double>(bytesReceived));

    if (logger_) {
        logger_->Trace("WorkflowNetworkReceiveStep", "Execute", "received", received ? "true" : "false");
        logger_->Trace("WorkflowNetworkReceiveStep", "Execute", "bytes_received", std::to_string(bytesReceived));
    }
}

}  // namespace sdl3cpp::services::impl
