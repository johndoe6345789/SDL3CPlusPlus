#pragma once

#include "services/interfaces/workflow_parameter_value.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <map>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Returns step's `name` parameter if it is a string, else `def`.
std::string FindStringParam(const WorkflowStepDefinition& step,
                            const char* name, const std::string& def);

enum class DebugMetricOperation { RECORD, AGGREGATE, RESET };
enum class DebugMetricAggregation { MIN, MAX, AVG, SUM, COUNT };

/// Running stats for one named metric.
struct DebugMetricData {
    std::vector<double> values;
    size_t recordCount = 0;
    double minValue    = 0.0;
    double maxValue    = 0.0;
    double sumValue    = 0.0;
};

/// Parses "record"/"aggregate"/"reset" case-insensitively.
/// @throws std::runtime_error on an unrecognized operation.
DebugMetricOperation ParseDebugMetricOperation(const std::string& opStr);

/// Parses "min"/"max"/"avg"/"sum"/"count" case-insensitively, defaulting
/// to AVG for anything unrecognized (matches debug.metrics' original
/// behavior of silently falling back rather than throwing).
DebugMetricAggregation ParseDebugMetricAggregation(const std::string& s);

/// Computes `type` over `data`; returns 0.0 if no values were recorded.
double AggregateDebugMetric(const DebugMetricData& data,
                            DebugMetricAggregation type);

/// Records `value` into `data`, updating count/sum/min/max in place.
void RecordDebugMetric(DebugMetricData& data, double value);

/// The metrics store shared by every debug.metrics step instance/call —
/// a workflow step is stateless per-invocation, so this is where
/// "record" values accumulate for a later "aggregate"/"reset" call.
std::map<std::string, DebugMetricData>& DebugMetricsStore();

}  // namespace sdl3cpp::services::impl
