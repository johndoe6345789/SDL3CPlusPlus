#include "services/interfaces/workflow/network_send_simulation.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

NetworkSendRequest ReadNetworkSendRequest(const WorkflowStepDefinition& step) {
    NetworkSendRequest request;

    if (auto p = step.parameters.find("connection_id");
        p != step.parameters.end() &&
        p->second.type == WorkflowParameterValue::Type::String) {
        request.connectionId = p->second.stringValue;
    } else {
        throw std::runtime_error(
            "Workflow network.send missing connection_id parameter");
    }

    if (auto p = step.parameters.find("payload");
        p != step.parameters.end() &&
        p->second.type == WorkflowParameterValue::Type::String) {
        request.payload = p->second.stringValue;
    } else {
        throw std::runtime_error(
            "Workflow network.send missing payload parameter");
    }

    if (auto p = step.parameters.find("priority");
        p != step.parameters.end() &&
        p->second.type == WorkflowParameterValue::Type::Number) {
        request.priority = static_cast<int>(p->second.numberValue);
    }

    return request;
}

NetworkSendOutcome SimulateNetworkSend(const NetworkSendRequest& request,
                                       const std::shared_ptr<ILogger>& logger) {
    bool sent = true;

    if (request.priority < 0 || request.priority > 10) {
        sent = false;
        if (logger) {
            logger->Trace("WorkflowNetworkSendStep", "Execute", "Error",
                          "Invalid priority");
        }
    }

    if (request.connectionId.empty()) {
        sent = false;
        if (logger) {
            logger->Trace("WorkflowNetworkSendStep", "Execute", "Error",
                          "Empty connection_id");
        }
    }

    NetworkSendOutcome outcome;
    outcome.sent      = sent;
    outcome.bytesSent = sent ? request.payload.size() : 0;
    return outcome;
}

}  // namespace sdl3cpp::services::impl
