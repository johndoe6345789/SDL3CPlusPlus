#pragma once

#include "services/interfaces/workflow/workflow_generic_steps/metric_parse.hpp"

#include <map>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Running stats for one named metric.
struct DebugMetricData {
    std::vector<double> values;
    size_t recordCount = 0;
    double minValue    = 0.0;
    double maxValue    = 0.0;
    double sumValue    = 0.0;
};

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
