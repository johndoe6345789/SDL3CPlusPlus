#include "services/interfaces/workflow/network_receive_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

namespace sdl3cpp::services::impl {

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

}  // namespace sdl3cpp::services::impl
