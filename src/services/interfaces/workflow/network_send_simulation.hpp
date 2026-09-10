#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// network.send's required and optional parameters, read straight off
/// the step definition.
struct NetworkSendRequest {
    std::string connectionId;
    std::string payload;
    int priority = 5;
};

/**
 * @brief Reads network.send's parameters from `step`.
 *
 * @throws std::runtime_error if connection_id or payload is missing.
 */
NetworkSendRequest ReadNetworkSendRequest(const WorkflowStepDefinition& step);

/// Outcome of a simulated send: whether it went through, and how many
/// bytes were "sent" (0 when validation failed).
struct NetworkSendOutcome {
    bool sent = false;
    uint64_t bytesSent = 0;
};

/**
 * @brief Validates `request` and simulates sending its payload, logging
 *        the reason through `logger` when validation fails.
 */
NetworkSendOutcome SimulateNetworkSend(const NetworkSendRequest& request,
                                       const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
