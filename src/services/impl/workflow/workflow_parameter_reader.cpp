#include "services/interfaces/workflow/workflow_parameter_reader.hpp"
#include "services/interfaces/workflow/workflow_parameter_value_parser.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowParameterReader::WorkflowParameterReader(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowParameterReader", "Constructor", "Entry");
    }
}

std::string WorkflowParameterReader::ReadRequiredString(
    const rapidjson::Value& object, const char* name) const {
    if (logger_) {
        logger_->Trace("WorkflowParameterReader", "ReadRequiredString",
                       "Entry");
    }
    if (!object.HasMember(name) || !object[name].IsString()) {
        throw std::runtime_error("Workflow member '" + std::string(name) +
                                 "' must be a string");
    }
    return object[name].GetString();
}

std::unordered_map<std::string, WorkflowParameterValue>
WorkflowParameterReader::ReadParameterMap(const rapidjson::Value& object,
                                          const char* name) const {
    if (logger_) {
        logger_->Trace("WorkflowParameterReader", "ReadParameterMap", "Entry");
    }

    std::unordered_map<std::string, WorkflowParameterValue> result;
    if (!object.HasMember(name)) {
        return result;
    }
    const auto& mapValue = object[name];
    if (!mapValue.IsObject()) {
        throw std::runtime_error("Workflow member '" + std::string(name) +
                                 "' must be an object");
    }

    for (auto it = mapValue.MemberBegin(); it != mapValue.MemberEnd(); ++it) {
        const std::string key = it->name.GetString();
        const auto& value     = it->value;

        // Skip nested 'inputs'/'outputs' objects - extracted separately by
        // the parser.
        if ((key == "inputs" || key == "outputs") && value.IsObject()) {
            continue;
        }
        result.emplace(key, ParseParameterValue(value, key));
    }
    return result;
}

}  // namespace sdl3cpp::services::impl
