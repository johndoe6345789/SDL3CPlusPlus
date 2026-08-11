#pragma once

#include "services/interfaces/workflow_definition.hpp"
#include "services/interfaces/i_logger.hpp"

#include <rapidjson/document.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/// Small focused class: Read workflow parameters from JSON
/// Single job: Parse RapidJSON values into WorkflowParameterValue maps
/// Keeps parameter reading logic under 100 LOC
class WorkflowParameterReader {
public:
    explicit WorkflowParameterReader(std::shared_ptr<ILogger> logger = nullptr);

    /// Read string→string map from JSON object
    /// Used for inputs/outputs mappings
    std::unordered_map<std::string, std::string> ReadStringMap(
        const rapidjson::Value& object,
        const char* memberName) const;

    /// Read string→WorkflowParameterValue map from JSON object
    /// Supports: string, number, bool, arrays (strings or numbers)
    /// Skips nested 'inputs'/'outputs' objects (extracted separately)
    std::unordered_map<std::string, WorkflowParameterValue> ReadParameterMap(
        const rapidjson::Value& object,
        const char* memberName) const;

    /// Read required string member
    std::string ReadRequiredString(const rapidjson::Value& object, const char* memberName) const;

    /// Read node ID (tries 'id' first, falls back to 'name')
    std::string ReadNodeId(const rapidjson::Value& node, size_t index) const;

    /// Read node plugin/type (tries 'plugin' first, falls back to 'type')
    std::string ReadNodePlugin(const rapidjson::Value& node, const std::string& nodeId) const;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
