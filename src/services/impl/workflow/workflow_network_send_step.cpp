#include "services/interfaces/workflow/workflow_network_send_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowNetworkSendStep::WorkflowNetworkSendStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowNetworkSendStep", "Constructor", "Entry");
    }
}

std::string WorkflowNetworkSendStep::GetPluginId() const {
    return "network.send";
}

void WorkflowNetworkSendStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowNetworkSendStep", "Execute", "Entry");
    }

    // Get connection ID parameter (required)
    std::string connectionId;
    if (auto connIdParam = step.parameters.find("connection_id"); connIdParam != step.parameters.end() &&
        connIdParam->second.type == WorkflowParameterValue::Type::String) {
        connectionId = connIdParam->second.stringValue;
    } else {
        throw std::runtime_error("Workflow network.send missing connection_id parameter");
    }

    // Get payload parameter (required)
    std::string payload;
    if (auto payloadParam = step.parameters.find("payload"); payloadParam != step.parameters.end() &&
        payloadParam->second.type == WorkflowParameterValue::Type::String) {
        payload = payloadParam->second.stringValue;
    } else {
        throw std::runtime_error("Workflow network.send missing payload parameter");
    }

    // Get priority parameter (optional, default: 5)
    int priority = 5;
    if (auto priorityParam = step.parameters.find("priority"); priorityParam != step.parameters.end() &&
        priorityParam->second.type == WorkflowParameterValue::Type::Number) {
        priority = static_cast<int>(priorityParam->second.numberValue);
    }

    if (logger_) {
        logger_->Trace("WorkflowNetworkSendStep", "Execute", "connection_id", connectionId);
        logger_->Trace("WorkflowNetworkSendStep", "Execute", "payload_size", std::to_string(payload.size()));
        logger_->Trace("WorkflowNetworkSendStep", "Execute", "priority", std::to_string(priority));
    }

    // Validate priority
    bool sent = true;
    if (priority < 0 || priority > 10) {
        sent = false;
        if (logger_) {
            logger_->Trace("WorkflowNetworkSendStep", "Execute", "Error", "Invalid priority");
        }
    }

    // Validate connection ID (basic check)
    if (connectionId.empty()) {
        sent = false;
        if (logger_) {
            logger_->Trace("WorkflowNetworkSendStep", "Execute", "Error", "Empty connection_id");
        }
    }

    // Simulate sending
    uint64_t bytesSent = sent ? payload.size() : 0;
    totalBytesSent_ += bytesSent;

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

    context.Set(sentKey, sent);
    context.Set(bytesSentKey, static_cast<double>(bytesSent));

    if (logger_) {
        logger_->Trace("WorkflowNetworkSendStep", "Execute", "sent", sent ? "true" : "false");
        logger_->Trace("WorkflowNetworkSendStep", "Execute", "bytes_sent", std::to_string(bytesSent));
    }
}

}  // namespace sdl3cpp::services::impl
