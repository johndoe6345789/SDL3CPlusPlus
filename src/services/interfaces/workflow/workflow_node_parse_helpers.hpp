#pragma once

#include "services/interfaces/workflow_definition.hpp"
#include "services/interfaces/workflow/workflow_parameter_reader.hpp"

#include <rapidjson/document.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Parses one entry of the workflow "nodes" array into a step.
 *
 * Also records `entry`'s "name" (if present) into `nameToId`, since n8n's
 * connections format references nodes by name rather than id.
 */
WorkflowStepDefinition ParseOneWorkflowNode(
    const WorkflowParameterReader& paramReader, const rapidjson::Value& entry,
    rapidjson::SizeType index,
    std::unordered_map<std::string, std::string>& nameToId);

/**
 * @brief Resolves n8n-style connections and returns `nodes` reordered to
 *        satisfy them.
 *
 * Falls back to `nodeOrder` (parse order) unmodified if `document` has no
 * connections.
 *
 * @throws std::runtime_error if a resolved node id has no matching entry
 *         in `nodes`.
 */
std::vector<WorkflowStepDefinition> SortWorkflowNodes(
    const rapidjson::Document& document,
    const std::vector<WorkflowStepDefinition>& nodes,
    const std::vector<std::string>& nodeOrder,
    const std::unordered_map<std::string, std::string>& nameToId);

}  // namespace sdl3cpp::services::impl
