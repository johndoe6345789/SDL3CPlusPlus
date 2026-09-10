#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_definition.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Locates and parses `<package>/workflows/<workflowName>.json`.
 *
 * Searches, in order: `./gameengine/packages`, `./packages`, then the same
 * two directory names under each ancestor of the current directory (up to
 * 5 levels up) — the first candidate that exists on disk and parses
 * successfully wins. Returns an empty WorkflowDefinition (after logging an
 * Error via `logger`, which may be null) if no candidate is found or every
 * candidate found fails to parse.
 */
WorkflowDefinition LoadChildWorkflow(const std::shared_ptr<ILogger>& logger,
                                     const std::string& package,
                                     const std::string& workflowName);

}  // namespace sdl3cpp::services::impl
