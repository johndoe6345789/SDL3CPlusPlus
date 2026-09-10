#include "services/interfaces/workflow/network_receive_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

NetworkReceiveParams ReadNetworkReceiveParams(
    const WorkflowStepDefinition& step) {
    NetworkReceiveParams out;

    auto connIdParam = step.parameters.find("connection_id");
    if (connIdParam != step.parameters.end() &&
        connIdParam->second.type == WorkflowParameterValue::Type::String) {
        out.connectionId = connIdParam->second.stringValue;
    } else {
        throw std::runtime_error(
            "Workflow network.receive missing connection_id parameter");
    }

    auto timeoutParam = step.parameters.find("timeout");
    if (timeoutParam != step.parameters.end() &&
        timeoutParam->second.type == WorkflowParameterValue::Type::Number) {
        out.timeout = static_cast<int>(timeoutParam->second.numberValue);
    }

    return out;
}

NetworkReceiveOutputKeys ResolveNetworkReceiveOutputKeys(
    const WorkflowStepDefinition& step) {
    WorkflowStepIoResolver resolver;
    NetworkReceiveOutputKeys out;

    try {
        out.receivedKey = resolver.GetRequiredOutputKey(step, "received");
    } catch (...) {
        // Use default.
    }
    try {
        out.payloadKey = resolver.GetRequiredOutputKey(step, "payload");
    } catch (...) {
        // Use default.
    }
    try {
        out.bytesReceivedKey =
            resolver.GetRequiredOutputKey(step, "bytes_received");
    } catch (...) {
        // Use default.
    }

    return out;
}

NetworkReceiveResult TryDequeueMessage(
    std::map<std::string, std::queue<std::string>>& messageQueues,
    const NetworkReceiveParams& params,
    const std::shared_ptr<ILogger>& logger) {
    NetworkReceiveResult out;

    if (params.connectionId.empty()) {
        if (logger) {
            logger->Trace("WorkflowNetworkReceiveStep", "Execute", "Error",
                          "Empty connection_id");
        }
        return out;
    }
    if (params.timeout < 0) {
        if (logger) {
            logger->Trace("WorkflowNetworkReceiveStep", "Execute", "Error",
                          "Negative timeout");
        }
        return out;
    }

    auto it = messageQueues.find(params.connectionId);
    if (it != messageQueues.end() && !it->second.empty()) {
        out.payload = it->second.front();
        it->second.pop();
        out.received      = true;
        out.bytesReceived = out.payload.size();
        if (logger) {
            logger->Trace("WorkflowNetworkReceiveStep", "Execute",
                          "Dequeued message", out.payload);
        }
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
