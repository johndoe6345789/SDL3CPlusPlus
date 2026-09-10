#pragma once

#include "services/interfaces/workflow_definition.hpp"

#include <rapidjson/document.h>

#include <string>

namespace sdl3cpp::services::impl {

/// Converts a RapidJSON array member into a WorkflowParameterValue string
/// list or number list. Throws std::runtime_error if an entry is neither a
/// string nor a number, or if the array mixes both.
WorkflowParameterValue ParseParameterArrayValue(const rapidjson::Value& value,
                                                const std::string& key);

}  // namespace sdl3cpp::services::impl
