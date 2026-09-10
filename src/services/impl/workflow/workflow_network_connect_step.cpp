#include "services/interfaces/workflow/workflow_network_connect_step.hpp"
#include "services/interfaces/workflow/network_connect_helpers.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowNetworkConnectStep::WorkflowNetworkConnectStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowNetworkConnectStep", "Constructor", "Entry");
    }
}

std::string WorkflowNetworkConnectStep::GetPluginId() const {
    return "network.connect";
}

void WorkflowNetworkConnectStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowNetworkConnectStep", "Execute", "Entry");
    }

    const NetworkConnectParams params = ReadNetworkConnectParams(step);
    if (logger_) {
        logger_->Trace("WorkflowNetworkConnectStep", "Execute", "host",
                       params.host);
        logger_->Trace("WorkflowNetworkConnectStep", "Execute", "port",
                       std::to_string(params.port));
        logger_->Trace("WorkflowNetworkConnectStep", "Execute", "timeout",
                       std::to_string(params.timeout));
    }

    const std::string connectionId =
        "conn_" + std::to_string(nextConnectionId_++);

    // Simulate connection establishment.
    bool connected = true;
    if (params.port < 1 || params.port > 65535) {
        connected = false;
        if (logger_) {
            logger_->Trace("WorkflowNetworkConnectStep", "Execute", "Error",
                           "Invalid port number");
        }
    }

    const NetworkConnectOutputKeys keys =
        ResolveNetworkConnectOutputKeys(step);
    context.Set(keys.connectionIdKey, connectionId);
    context.Set(keys.connectedKey, connected);

    if (logger_) {
        logger_->Trace("WorkflowNetworkConnectStep", "Execute",
                       "connection_id", connectionId);
        logger_->Trace("WorkflowNetworkConnectStep", "Execute", "connected",
                       connected ? "true" : "false");
    }
}

}  // namespace sdl3cpp::services::impl
