#include "services/interfaces/workflow/workflow_network_connect_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowNetworkConnectStep::WorkflowNetworkConnectStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowNetworkConnectStep", "Constructor", "Entry");
    }
}

std::string WorkflowNetworkConnectStep::GetPluginId() const {
    return "network.connect";
}

void WorkflowNetworkConnectStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowNetworkConnectStep", "Execute", "Entry");
    }

    // Get host parameter (optional, default: "localhost")
    std::string host = "localhost";
    if (auto hostParam = step.parameters.find("host"); hostParam != step.parameters.end() &&
        hostParam->second.type == WorkflowParameterValue::Type::String) {
        host = hostParam->second.stringValue;
    }

    // Get port parameter (optional, default: 8080)
    int port = 8080;
    if (auto portParam = step.parameters.find("port"); portParam != step.parameters.end() &&
        portParam->second.type == WorkflowParameterValue::Type::Number) {
        port = static_cast<int>(portParam->second.numberValue);
    }

    // Get timeout parameter (optional, in milliseconds, default: 5000)
    int timeout = 5000;
    if (auto timeoutParam = step.parameters.find("timeout"); timeoutParam != step.parameters.end() &&
        timeoutParam->second.type == WorkflowParameterValue::Type::Number) {
        timeout = static_cast<int>(timeoutParam->second.numberValue);
    }

    if (logger_) {
        logger_->Trace("WorkflowNetworkConnectStep", "Execute", "host", host);
        logger_->Trace("WorkflowNetworkConnectStep", "Execute", "port", std::to_string(port));
        logger_->Trace("WorkflowNetworkConnectStep", "Execute", "timeout", std::to_string(timeout));
    }

    // Generate connection ID
    std::string connectionId = "conn_" + std::to_string(nextConnectionId_++);

    // Simulate connection establishment
    bool connected = true;
    if (port < 1 || port > 65535) {
        connected = false;
        if (logger_) {
            logger_->Trace("WorkflowNetworkConnectStep", "Execute", "Error", "Invalid port number");
        }
    }

    // Get output keys from step definition or use defaults
    WorkflowStepIoResolver resolver;
    std::string connectionIdKey = "network.connection_id";
    std::string connectedKey = "network.connected";

    try {
        connectionIdKey = resolver.GetRequiredOutputKey(step, "connection_id");
    } catch (...) {
        // Use default
    }

    try {
        connectedKey = resolver.GetRequiredOutputKey(step, "connected");
    } catch (...) {
        // Use default
    }

    context.Set(connectionIdKey, connectionId);
    context.Set(connectedKey, connected);

    if (logger_) {
        logger_->Trace("WorkflowNetworkConnectStep", "Execute", "connection_id", connectionId);
        logger_->Trace("WorkflowNetworkConnectStep", "Execute", "connected", connected ? "true" : "false");
    }
}

}  // namespace sdl3cpp::services::impl
