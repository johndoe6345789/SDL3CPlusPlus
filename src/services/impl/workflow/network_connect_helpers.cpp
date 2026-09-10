#include "services/interfaces/workflow/network_connect_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

namespace sdl3cpp::services::impl {

NetworkConnectParams ReadNetworkConnectParams(
    const WorkflowStepDefinition& step) {
    NetworkConnectParams out;

    if (auto hostParam = step.parameters.find("host");
        hostParam != step.parameters.end() &&
        hostParam->second.type == WorkflowParameterValue::Type::String) {
        out.host = hostParam->second.stringValue;
    }
    if (auto portParam = step.parameters.find("port");
        portParam != step.parameters.end() &&
        portParam->second.type == WorkflowParameterValue::Type::Number) {
        out.port = static_cast<int>(portParam->second.numberValue);
    }
    if (auto timeoutParam = step.parameters.find("timeout");
        timeoutParam != step.parameters.end() &&
        timeoutParam->second.type == WorkflowParameterValue::Type::Number) {
        out.timeout = static_cast<int>(timeoutParam->second.numberValue);
    }

    return out;
}

NetworkConnectOutputKeys ResolveNetworkConnectOutputKeys(
    const WorkflowStepDefinition& step) {
    WorkflowStepIoResolver resolver;
    NetworkConnectOutputKeys out;

    try {
        out.connectionIdKey =
            resolver.GetRequiredOutputKey(step, "connection_id");
    } catch (...) {
        // Use default.
    }
    try {
        out.connectedKey = resolver.GetRequiredOutputKey(step, "connected");
    } catch (...) {
        // Use default.
    }

    return out;
}

}  // namespace sdl3cpp::services::impl
