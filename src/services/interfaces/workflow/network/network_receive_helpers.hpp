#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <queue>
#include <string>

namespace sdl3cpp::services::impl {

/// network.receive's parsed parameters.
struct NetworkReceiveParams {
    std::string connectionId;
    int timeout = 1000;
};

/// Reads `connection_id` (required) and `timeout` (optional, default
/// 1000ms) from step parameters. Throws std::runtime_error if
/// connection_id is missing.
NetworkReceiveParams ReadNetworkReceiveParams(
    const WorkflowStepDefinition& step);

/// network.receive's output context keys, each defaulting to
/// "network.<name>" if the step doesn't declare an explicit output.
struct NetworkReceiveOutputKeys {
    std::string receivedKey      = "network.received";
    std::string payloadKey       = "network.payload";
    std::string bytesReceivedKey = "network.bytes_received";
};

NetworkReceiveOutputKeys ResolveNetworkReceiveOutputKeys(
    const WorkflowStepDefinition& step);

/// The outcome of attempting to dequeue a message for one connection.
struct NetworkReceiveResult {
    bool received = false;
    std::string payload;
    uint64_t bytesReceived = 0;
};

/**
 * @brief Dequeues the next queued message for `params.connectionId`, if
 * any, from `messageQueues`.
 *
 * Returns received=false without touching the queue if connectionId is
 * empty or timeout is negative (both treated as invalid input, not
 * errors worth throwing over).
 */
NetworkReceiveResult TryDequeueMessage(
    std::map<std::string, std::queue<std::string>>& messageQueues,
    const NetworkReceiveParams& params, const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
