#include "services/interfaces/workflow/workflow_definition_parser.hpp"

#include <rapidjson/document.h>

#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

void WorkflowDefinitionParser::ParseVariables(
    const rapidjson::Document& document,
    WorkflowDefinition& workflow) const {
    if (logger_) {
        logger_->Trace("WorkflowDefinitionParser", "ParseVariables", "Entry");
    }

    if (!document.HasMember("variables") || !document["variables"].IsObject()) {
        return;
    }

    const auto& varsObj = document["variables"];

    for (auto it = varsObj.MemberBegin(); it != varsObj.MemberEnd(); ++it) {
        if (!it->value.IsObject()) {
            continue;  // Skip invalid variable definitions
        }

        WorkflowVariable var;
        var.name = it->name.GetString();

        const auto& varDef = it->value;
        if (varDef.HasMember("type") && varDef["type"].IsString()) {
            var.type = varDef["type"].GetString();
        }
        if (varDef.HasMember("description") && varDef["description"].IsString()) {
            var.description = varDef["description"].GetString();
        }
        if (varDef.HasMember("defaultValue")) {
            // Store defaultValue as string representation
            if (varDef["defaultValue"].IsString()) {
                var.defaultValue = varDef["defaultValue"].GetString();
            } else if (varDef["defaultValue"].IsNumber()) {
                // Check if it's an integer to avoid .000000 suffix
                if (varDef["defaultValue"].IsInt() || varDef["defaultValue"].IsInt64()) {
                    var.defaultValue = std::to_string(varDef["defaultValue"].GetInt64());
                } else {
                    var.defaultValue = std::to_string(varDef["defaultValue"].GetDouble());
                }
            } else if (varDef["defaultValue"].IsBool()) {
                var.defaultValue = varDef["defaultValue"].GetBool() ? "true" : "false";
            }
        }
        if (varDef.HasMember("required") && varDef["required"].IsBool()) {
            var.required = varDef["required"].GetBool();
        }

        workflow.variables[var.name] = std::move(var);
    }
}

}  // namespace sdl3cpp::services::impl
