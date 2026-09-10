#include "services/interfaces/workflow/workflow_generic_steps/metric_parse.hpp"

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

}  // namespace sdl3cpp::services::impl
