#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_definition.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Returns step's required `name` string parameter, or throws
/// std::runtime_error(errorMsg) if it is not present.
const std::string& RequireStringParam(const WorkflowStepDefinition& step,
                                      const char* name,
                                      const char* errorMsg);

/**
 * @brief Loads `<package>/workflows/<workflowName>.json` and parses it.
 *
 * Searches "gameengine/packages" and "packages" under the current working
 * directory, then the same two names under each ancestor directory (up to
 * 5 levels up), returning the first match parsed via
 * WorkflowDefinitionParser.
 *
 * @return An empty WorkflowDefinition (empty `steps`) if no match was
 *         found in any searched directory; logs an error via `logger`
 *         when that happens.
 */
WorkflowDefinition LoadPackageWorkflow(const std::shared_ptr<ILogger>& logger,
                                       const std::string& package,
                                       const std::string& workflowName);

}  // namespace sdl3cpp::services::impl
