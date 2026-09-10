#include "services/interfaces/workflow/network_receive_helpers.hpp"

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

}  // namespace sdl3cpp::services::impl
