#include "services/interfaces/workflow/workflow_parameter_reader.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

std::unordered_map<std::string, std::string>
WorkflowParameterReader::ReadStringMap(const rapidjson::Value& object,
                                       const char* name) const {
    if (logger_) {
        logger_->Trace("WorkflowParameterReader", "ReadStringMap", "Entry");
    }

    std::unordered_map<std::string, std::string> result;
    if (!object.HasMember(name)) {
        return result;
    }
    const auto& mapValue = object[name];
    if (!mapValue.IsObject()) {
        throw std::runtime_error("Workflow member '" + std::string(name) +
                                 "' must be an object");
    }
    for (auto it = mapValue.MemberBegin(); it != mapValue.MemberEnd(); ++it) {
        if (!it->value.IsString()) {
            throw std::runtime_error("Workflow map '" + std::string(name) +
                                     "' must map to strings");
        }
        result[it->name.GetString()] = it->value.GetString();
    }
    return result;
}

std::string WorkflowParameterReader::ReadNodeId(const rapidjson::Value& node,
                                                 size_t index) const {
    if (logger_) {
        logger_->Trace("WorkflowParameterReader", "ReadNodeId", "Entry");
    }
    if (node.HasMember("id") && node["id"].IsString()) {
        return node["id"].GetString();
    }
    if (node.HasMember("name") && node["name"].IsString()) {
        return node["name"].GetString();
    }
    throw std::runtime_error("Workflow node[" + std::to_string(index) +
                             "] requires string id or name");
}

std::string WorkflowParameterReader::ReadNodePlugin(
    const rapidjson::Value& node, const std::string& nodeId) const {
    if (logger_) {
        logger_->Trace("WorkflowParameterReader", "ReadNodePlugin", "Entry");
    }
    if (node.HasMember("plugin") && node["plugin"].IsString()) {
        return node["plugin"].GetString();
    }
    if (node.HasMember("type") && node["type"].IsString()) {
        return node["type"].GetString();
    }
    throw std::runtime_error("Workflow node '" + nodeId +
                             "' requires string plugin or type");
}

}  // namespace sdl3cpp::services::impl
