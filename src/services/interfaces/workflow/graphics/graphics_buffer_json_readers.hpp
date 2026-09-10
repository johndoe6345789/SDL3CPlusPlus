#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Converts a JSON array of numbers into a flat byte array. Throws
/// std::runtime_error if the array is missing, empty, or non-numeric.
std::vector<uint8_t> ReadVertexBytesFromContext(const WorkflowContext& context,
                                                const std::string& key);

/// Converts a JSON array of numbers into a uint16 index array. Throws
/// std::runtime_error if the array is missing, empty, or non-numeric.
std::vector<uint16_t> ReadIndexValuesFromContext(const WorkflowContext& context,
                                                 const std::string& key);

}  // namespace sdl3cpp::services::impl
