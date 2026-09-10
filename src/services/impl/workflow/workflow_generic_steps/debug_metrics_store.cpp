#include "services/interfaces/workflow/workflow_generic_steps/debug_metrics_store.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace sdl3cpp::services::impl {
namespace {

std::string ToLowerCopy(const std::string& s) {
    std::string normalized = s;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return normalized;
}

}  // namespace

std::string FindStringParam(const WorkflowStepDefinition& step,
                            const char* name, const std::string& def) {
    auto it = step.parameters.find(name);
    if (it != step.parameters.end() &&
        it->second.type == WorkflowParameterValue::Type::String) {
        return it->second.stringValue;
    }
    return def;
}

DebugMetricOperation ParseDebugMetricOperation(const std::string& opStr) {
    const std::string normalized = ToLowerCopy(opStr);
    if (normalized == "record") return DebugMetricOperation::RECORD;
    if (normalized == "aggregate") return DebugMetricOperation::AGGREGATE;
    if (normalized == "reset") return DebugMetricOperation::RESET;
    throw std::runtime_error("debug.metrics unknown operation: " + opStr);
}

DebugMetricAggregation ParseDebugMetricAggregation(const std::string& s) {
    const std::string normalized = ToLowerCopy(s);
    if (normalized == "min") return DebugMetricAggregation::MIN;
    if (normalized == "max") return DebugMetricAggregation::MAX;
    if (normalized == "avg") return DebugMetricAggregation::AVG;
    if (normalized == "sum") return DebugMetricAggregation::SUM;
    if (normalized == "count") return DebugMetricAggregation::COUNT;
    return DebugMetricAggregation::AVG;
}

double AggregateDebugMetric(const DebugMetricData& data,
                            DebugMetricAggregation type) {
    if (data.values.empty()) {
        return 0.0;
    }

    switch (type) {
        case DebugMetricAggregation::MIN:
            return data.minValue;
        case DebugMetricAggregation::MAX:
            return data.maxValue;
        case DebugMetricAggregation::SUM:
            return data.sumValue;
        case DebugMetricAggregation::COUNT:
            return static_cast<double>(data.recordCount);
        case DebugMetricAggregation::AVG:
            if (data.recordCount == 0) return 0.0;
            return data.sumValue / static_cast<double>(data.recordCount);
    }
    return 0.0;
}

void RecordDebugMetric(DebugMetricData& data, double value) {
    data.values.push_back(value);
    data.recordCount++;
    data.sumValue += value;

    if (data.recordCount == 1) {
        data.minValue = value;
        data.maxValue = value;
    } else {
        if (value < data.minValue) data.minValue = value;
        if (value > data.maxValue) data.maxValue = value;
    }
}

std::map<std::string, DebugMetricData>& DebugMetricsStore() {
    static std::map<std::string, DebugMetricData> store;
    return store;
}

}  // namespace sdl3cpp::services::impl
