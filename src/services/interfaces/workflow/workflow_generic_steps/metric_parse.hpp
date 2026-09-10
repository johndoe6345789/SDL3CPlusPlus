#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Returns step's `name` parameter if it is a string, else `def`.
std::string FindStringParam(const WorkflowStepDefinition& step,
                            const char* name, const std::string& def);

enum class DebugMetricOperation { RECORD, AGGREGATE, RESET };
enum class DebugMetricAggregation { MIN, MAX, AVG, SUM, COUNT };

/// Parses "record"/"aggregate"/"reset" case-insensitively.
/// @throws std::runtime_error on an unrecognized operation.
DebugMetricOperation ParseDebugMetricOperation(const std::string& opStr);

/// Parses "min"/"max"/"avg"/"sum"/"count" case-insensitively, defaulting
/// to AVG for anything unrecognized (matches debug.metrics' original
/// behavior of silently falling back rather than throwing).
DebugMetricAggregation ParseDebugMetricAggregation(const std::string& s);

}  // namespace sdl3cpp::services::impl
