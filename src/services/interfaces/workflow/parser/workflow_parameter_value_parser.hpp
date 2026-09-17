#pragma once

#include "services/interfaces/workflow_definition.hpp"

#include <rapidjson/document.h>

#include <string>

namespace sdl3cpp::services::impl {

/// Expands `${env:VAR_NAME}` placeholders in a workflow string parameter.
/// Resolved at JSON load time so every step reads already-substituted
/// values. Unset variables expand to empty string (the step's
/// missing-value handling then takes over with its own error message -
/// keeps this layer policy-free). Shared by scalar and array parsing.
std::string ExpandEnvPlaceholders(const std::string& input);

/// Converts one RapidJSON member value into a WorkflowParameterValue:
/// string (with env placeholders expanded), bool, number, or an array
/// (delegated to ParseParameterArrayValue). Throws std::runtime_error for
/// any other JSON type.
WorkflowParameterValue ParseParameterValue(const rapidjson::Value& value,
                                           const std::string& key);

}  // namespace sdl3cpp::services::impl
