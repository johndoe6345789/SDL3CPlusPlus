#include "services/interfaces/workflow/workflow_connection_main_parser.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {
namespace {

void ReadBranchConnections(
    const rapidjson::Value& branch, const std::string& fromNode,
    std::vector<std::pair<std::string, std::string>>& edges) {
    for (const auto& connection : branch.GetArray()) {
        if (!connection.IsObject() || !connection.HasMember("node") ||
            !connection["node"].IsString()) {
            throw std::runtime_error(
                "Workflow connection entries for '" + fromNode +
                "' require a node string");
        }
        edges.emplace_back(fromNode, connection["node"].GetString());
    }
}

}  // namespace

void ParseMainConnections(
    const rapidjson::Value& mainValue, const std::string& fromNode,
    std::vector<std::pair<std::string, std::string>>& edges) {
    // Support both n8n format (object with numeric keys) and simple array
    // format.
    if (mainValue.IsObject()) {
        // n8n format: "main": { "0": [...], "1": [...] }
        for (auto branchIt = mainValue.MemberBegin();
            branchIt != mainValue.MemberEnd(); ++branchIt) {
            if (!branchIt->value.IsArray()) {
                throw std::runtime_error(
                    "Workflow connections.main[" +
                    std::string(branchIt->name.GetString()) + "] for '" +
                    fromNode + "' must be an array");
            }
            ReadBranchConnections(branchIt->value, fromNode, edges);
        }
    } else if (mainValue.IsArray()) {
        // Simple array format: "main": [[...]]
        for (const auto& branch : mainValue.GetArray()) {
            if (!branch.IsArray()) {
                throw std::runtime_error(
                    "Workflow connections.main entries for '" + fromNode +
                    "' must be arrays");
            }
            ReadBranchConnections(branch, fromNode, edges);
        }
    } else {
        throw std::runtime_error("Workflow connections.main for '" +
                                 fromNode + "' must be an object or array");
    }
}

}  // namespace sdl3cpp::services::impl
