#include "services/interfaces/workflow/network_receive_helpers.hpp"

namespace sdl3cpp::services::impl {

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
