#include "workflow_definition_parser.hpp"
#include "json_config_document_parser.hpp"

#include <rapidjson/document.h>

#include <stdexcept>
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
}  // namespace

WorkflowDefinition WorkflowDefinitionParser::ParseFile(const std::filesystem::path& path) const {
    json_config::JsonConfigDocumentParser parser;
    rapidjson::Document document = parser.Parse(path, "workflow file");

    if (!document.HasMember("steps") || !document["steps"].IsArray()) {
        throw std::runtime_error("Workflow must contain a 'steps' array");
    }

    WorkflowDefinition workflow;
    if (document.HasMember("template")) {
        if (!document["template"].IsString()) {
            throw std::runtime_error("Workflow member 'template' must be a string");
        }
        workflow.templateName = document["template"].GetString();
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
        workflow.steps.push_back(std::move(step));
    }

    return workflow;
}

}  // namespace sdl3cpp::services::impl
