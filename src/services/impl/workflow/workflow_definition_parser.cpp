#include "workflow_definition_parser.hpp"
#include "../config/json_config_document_parser.hpp"

#include <rapidjson/document.h>

#include <stdexcept>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

namespace sdl3cpp::services::impl {

namespace {
std::string ReadRequiredString(const rapidjson::Value& object, const char* name) {
    if (!object.HasMember(name) || !object[name].IsString()) {
        throw std::runtime_error("Workflow member '" + std::string(name) + "' must be a string");
    }
    return object[name].GetString();
}

std::unordered_map<std::string, std::string> ReadStringMap(const rapidjson::Value& object,
                                                           const char* name) {
    std::unordered_map<std::string, std::string> result;
    if (!object.HasMember(name)) {
        return result;
    }
    const auto& mapValue = object[name];
    if (!mapValue.IsObject()) {
        throw std::runtime_error("Workflow member '" + std::string(name) + "' must be an object");
    }
    for (auto it = mapValue.MemberBegin(); it != mapValue.MemberEnd(); ++it) {
        if (!it->value.IsString()) {
            throw std::runtime_error("Workflow map '" + std::string(name) + "' must map to strings");
        }
        result[it->name.GetString()] = it->value.GetString();
    }
    return result;
}

std::unordered_map<std::string, WorkflowParameterValue> ReadParameterMap(const rapidjson::Value& object,
                                                                          const char* name) {
    std::unordered_map<std::string, WorkflowParameterValue> result;
    if (!object.HasMember(name)) {
        return result;
    }
    const auto& mapValue = object[name];
    if (!mapValue.IsObject()) {
        throw std::runtime_error("Workflow member '" + std::string(name) + "' must be an object");
    }
    for (auto it = mapValue.MemberBegin(); it != mapValue.MemberEnd(); ++it) {
        const std::string key = it->name.GetString();
        const auto& value = it->value;
        if (value.IsString()) {
            result.emplace(key, WorkflowParameterValue::FromString(value.GetString()));
            continue;
        }
        if (value.IsBool()) {
            result.emplace(key, WorkflowParameterValue::FromBool(value.GetBool()));
            continue;
        }
        if (value.IsNumber()) {
            result.emplace(key, WorkflowParameterValue::FromNumber(value.GetDouble()));
            continue;
        }
        if (value.IsArray()) {
            std::vector<std::string> stringItems;
            std::vector<double> numberItems;
            for (rapidjson::SizeType i = 0; i < value.Size(); ++i) {
                const auto& entry = value[i];
                if (entry.IsString()) {
                    stringItems.emplace_back(entry.GetString());
                } else if (entry.IsNumber()) {
                    numberItems.emplace_back(entry.GetDouble());
                } else {
                    throw std::runtime_error("Workflow parameter '" + key + "' must contain strings or numbers");
                }
            }
            if (!stringItems.empty() && !numberItems.empty()) {
                throw std::runtime_error("Workflow parameter '" + key + "' cannot mix string and number values");
            }
            if (!stringItems.empty() || value.Empty()) {
                result.emplace(key, WorkflowParameterValue::FromStringList(std::move(stringItems)));
            } else {
                result.emplace(key, WorkflowParameterValue::FromNumberList(std::move(numberItems)));
            }
            continue;
        }
        throw std::runtime_error("Workflow parameter '" + key + "' must be a string, number, bool, or array");
    }
    return result;
}

std::string ReadNodeId(const rapidjson::Value& node, size_t index) {
    if (node.HasMember("id") && node["id"].IsString()) {
        return node["id"].GetString();
    }
    if (node.HasMember("name") && node["name"].IsString()) {
        return node["name"].GetString();
    }
    throw std::runtime_error("Workflow node[" + std::to_string(index) + "] requires string id or name");
}

std::string ReadNodePlugin(const rapidjson::Value& node, const std::string& nodeId) {
    if (node.HasMember("plugin") && node["plugin"].IsString()) {
        return node["plugin"].GetString();
    }
    if (node.HasMember("type") && node["type"].IsString()) {
        return node["type"].GetString();
    }
    throw std::runtime_error("Workflow node '" + nodeId + "' requires string plugin or type");
}

std::vector<std::pair<std::string, std::string>> ReadConnections(const rapidjson::Value& document) {
    if (!document.HasMember("connections")) {
        return {};
    }
    const auto& connectionsValue = document["connections"];
    if (!connectionsValue.IsObject()) {
        throw std::runtime_error("Workflow member 'connections' must be an object");
    }
    std::vector<std::pair<std::string, std::string>> edges;
    for (auto it = connectionsValue.MemberBegin(); it != connectionsValue.MemberEnd(); ++it) {
        const std::string fromNode = it->name.GetString();
        if (!it->value.IsObject()) {
            throw std::runtime_error("Workflow connections for '" + fromNode + "' must be an object");
        }
        if (!it->value.HasMember("main")) {
            continue;
        }
        const auto& mainValue = it->value["main"];
        if (!mainValue.IsArray()) {
            throw std::runtime_error("Workflow connections.main for '" + fromNode + "' must be an array");
        }
        for (const auto& branch : mainValue.GetArray()) {
            if (!branch.IsArray()) {
                throw std::runtime_error("Workflow connections.main entries for '" + fromNode + "' must be arrays");
            }
            for (const auto& connection : branch.GetArray()) {
                if (!connection.IsObject() || !connection.HasMember("node") || !connection["node"].IsString()) {
                    throw std::runtime_error("Workflow connection entries for '" + fromNode + "' require a node string");
                }
                edges.emplace_back(fromNode, connection["node"].GetString());
            }
        }
    }
    return edges;
}

std::vector<std::string> SortNodesByConnections(
    const std::vector<std::string>& nodeOrder,
    const std::vector<std::pair<std::string, std::string>>& edges) {
    std::unordered_map<std::string, size_t> indexById;
    std::unordered_map<std::string, size_t> indegree;
    std::unordered_map<std::string, std::vector<std::string>> adjacency;
    indexById.reserve(nodeOrder.size());
    indegree.reserve(nodeOrder.size());
    adjacency.reserve(nodeOrder.size());
    for (size_t i = 0; i < nodeOrder.size(); ++i) {
        indexById.emplace(nodeOrder[i], i);
        indegree.emplace(nodeOrder[i], 0);
        adjacency.emplace(nodeOrder[i], std::vector<std::string>{});
    }

    for (const auto& edge : edges) {
        const auto fromIt = indexById.find(edge.first);
        if (fromIt == indexById.end()) {
            throw std::runtime_error("Workflow connection references unknown node '" + edge.first + "'");
        }
        const auto toIt = indexById.find(edge.second);
        if (toIt == indexById.end()) {
            throw std::runtime_error("Workflow connection references unknown node '" + edge.second + "'");
        }
        adjacency[edge.first].push_back(edge.second);
        ++indegree[edge.second];
    }

    std::set<std::pair<size_t, std::string>> ready;
    for (const auto& nodeId : nodeOrder) {
        if (indegree[nodeId] == 0u) {
            ready.emplace(indexById[nodeId], nodeId);
        }
    }

    std::vector<std::string> ordered;
    ordered.reserve(nodeOrder.size());
    while (!ready.empty()) {
        auto it = ready.begin();
        const std::string nodeId = it->second;
        ready.erase(it);
        ordered.push_back(nodeId);
        for (const auto& next : adjacency[nodeId]) {
            auto indegreeIt = indegree.find(next);
            if (indegreeIt == indegree.end()) {
                continue;
            }
            if (--indegreeIt->second == 0u) {
                ready.emplace(indexById[next], next);
            }
        }
    }

    if (ordered.size() != nodeOrder.size()) {
        throw std::runtime_error("Workflow connections contain a cycle");
    }

    return ordered;
}
}  // namespace

WorkflowDefinition WorkflowDefinitionParser::ParseFile(const std::filesystem::path& path) const {
    json_config::JsonConfigDocumentParser parser;
    rapidjson::Document document = parser.Parse(path, "workflow file");

    const bool hasSteps = document.HasMember("steps");
    const bool hasNodes = document.HasMember("nodes");
    if (hasSteps && hasNodes) {
        throw std::runtime_error("Workflow cannot define both 'steps' and 'nodes'");
    }
    if (!hasSteps && !hasNodes) {
        throw std::runtime_error("Workflow must contain a 'steps' array or 'nodes' array");
    }

    WorkflowDefinition workflow;
    if (document.HasMember("template")) {
        workflow.templateName = ReadRequiredString(document, "template");
    }

    if (hasSteps) {
        if (!document["steps"].IsArray()) {
            throw std::runtime_error("Workflow must contain a 'steps' array");
        }
        for (const auto& entry : document["steps"].GetArray()) {
            if (!entry.IsObject()) {
                throw std::runtime_error("Workflow steps must be objects");
            }
            WorkflowStepDefinition step;
            step.id = ReadRequiredString(entry, "id");
            step.plugin = ReadRequiredString(entry, "plugin");
            step.inputs = ReadStringMap(entry, "inputs");
            step.outputs = ReadStringMap(entry, "outputs");
            step.parameters = ReadParameterMap(entry, "parameters");
            workflow.steps.push_back(std::move(step));
        }
        return workflow;
    }

    if (!document["nodes"].IsArray()) {
        throw std::runtime_error("Workflow must contain a 'nodes' array");
    }

    std::vector<WorkflowStepDefinition> nodes;
    std::vector<std::string> nodeOrder;
    for (rapidjson::SizeType i = 0; i < document["nodes"].Size(); ++i) {
        const auto& entry = document["nodes"][i];
        if (!entry.IsObject()) {
            throw std::runtime_error("Workflow nodes must be objects");
        }
        WorkflowStepDefinition step;
        step.id = ReadNodeId(entry, i);
        step.plugin = ReadNodePlugin(entry, step.id);
        step.inputs = ReadStringMap(entry, "inputs");
        step.outputs = ReadStringMap(entry, "outputs");
        step.parameters = ReadParameterMap(entry, "parameters");
        nodes.push_back(step);
        nodeOrder.push_back(step.id);
    }

    const auto edges = ReadConnections(document);
    std::vector<std::string> orderedIds = edges.empty()
        ? nodeOrder
        : SortNodesByConnections(nodeOrder, edges);

    std::unordered_map<std::string, WorkflowStepDefinition> nodeMap;
    nodeMap.reserve(nodes.size());
    for (const auto& node : nodes) {
        nodeMap.emplace(node.id, node);
    }
    workflow.steps.reserve(nodes.size());
    for (const auto& nodeId : orderedIds) {
        auto it = nodeMap.find(nodeId);
        if (it == nodeMap.end()) {
            throw std::runtime_error("Workflow nodes missing entry for '" + nodeId + "'");
        }
        workflow.steps.push_back(it->second);
    }

    return workflow;
}

}  // namespace sdl3cpp::services::impl
