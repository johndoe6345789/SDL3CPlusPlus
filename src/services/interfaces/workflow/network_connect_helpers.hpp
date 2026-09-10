#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// network.connect's parsed parameters, each with the original
/// defaults.
struct NetworkConnectParams {
    std::string host = "localhost";
    int port          = 8080;
    int timeout       = 5000;
};

NetworkConnectParams ReadNetworkConnectParams(
    const WorkflowStepDefinition& step);

/// network.connect's output context keys, each defaulting to
/// "network.<name>" if the step doesn't declare an explicit output.
struct NetworkConnectOutputKeys {
    std::string connectionIdKey = "network.connection_id";
    std::string connectedKey    = "network.connected";
};

NetworkConnectOutputKeys ResolveNetworkConnectOutputKeys(
    const WorkflowStepDefinition& step);

}  // namespace sdl3cpp::services::impl
