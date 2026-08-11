#include "services/interfaces/workflow/workflow_generic_steps/workflow_debug_metrics_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <cctype>
#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

// Static member initialization
std::map<std::string, WorkflowDebugMetricsStep::MetricData> WorkflowDebugMetricsStep::globalMetrics_;

WorkflowDebugMetricsStep::WorkflowDebugMetricsStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDebugMetricsStep::GetPluginId() const {
    return "debug.metrics";
}

WorkflowDebugMetricsStep::Operation WorkflowDebugMetricsStep::ParseOperation(const std::string& opStr) const {
    std::string normalized = opStr;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (normalized == "record") return Operation::RECORD;
    if (normalized == "aggregate") return Operation::AGGREGATE;
    if (normalized == "reset") return Operation::RESET;

    throw std::runtime_error("debug.metrics unknown operation: " + opStr);
}

WorkflowDebugMetricsStep::AggregationType WorkflowDebugMetricsStep::ParseAggregationType(const std::string& aggStr) const {
    std::string normalized = aggStr;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (normalized == "min") return AggregationType::MIN;
    if (normalized == "max") return AggregationType::MAX;
    if (normalized == "avg") return AggregationType::AVG;
    if (normalized == "sum") return AggregationType::SUM;
    if (normalized == "count") return AggregationType::COUNT;

    return AggregationType::AVG;
}

double WorkflowDebugMetricsStep::AggregateMetric(const MetricData& data, AggregationType type) const {
    if (data.values.empty()) {
        return 0.0;
    }

    switch (type) {
        case AggregationType::MIN:
            return data.minValue;

        case AggregationType::MAX:
            return data.maxValue;

        case AggregationType::SUM:
            return data.sumValue;

        case AggregationType::COUNT:
            return static_cast<double>(data.recordCount);

        case AggregationType::AVG:
            if (data.recordCount == 0) return 0.0;
            return data.sumValue / static_cast<double>(data.recordCount);
    }

    return 0.0;
}

void WorkflowDebugMetricsStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver ioResolver;

    std::string metricNameKey = ioResolver.GetRequiredInputKey(step, "metric_name");
    const auto* metricName = context.TryGet<std::string>(metricNameKey);
    if (!metricName) {
        throw std::runtime_error("debug.metrics missing input '" + metricNameKey + "'");
    }

    // Get operation parameter (default: "record")
    std::string operation = "record";
    auto opIt = step.parameters.find("operation");
    if (opIt != step.parameters.end() && opIt->second.type == WorkflowParameterValue::Type::String) {
        operation = opIt->second.stringValue;
    }

    const Operation op = ParseOperation(operation);

    if (op == Operation::RECORD) {
        std::string metricValueKey = ioResolver.GetRequiredInputKey(step, "metric_value");
        const auto* metricValue = context.TryGet<double>(metricValueKey);
        if (!metricValue) {
            throw std::runtime_error("debug.metrics missing input '" + metricValueKey + "' for record operation");
        }

        MetricData& data = globalMetrics_[*metricName];
        data.values.push_back(*metricValue);
        data.recordCount++;
        data.sumValue += *metricValue;

        if (data.recordCount == 1) {
            data.minValue = *metricValue;
            data.maxValue = *metricValue;
        } else {
            if (*metricValue < data.minValue) {
                data.minValue = *metricValue;
            }
            if (*metricValue > data.maxValue) {
                data.maxValue = *metricValue;
            }
        }

        if (logger_) {
            logger_->Trace("WorkflowDebugMetricsStep", "Record",
                          "metric=" + *metricName + ", value=" + std::to_string(*metricValue),
                          "Recorded metric value");
        }

    } else if (op == Operation::AGGREGATE) {
        // Get aggregation type (default: "avg")
        std::string aggTypeStr = "avg";
        auto aggIt = step.parameters.find("agg_type");
        if (aggIt != step.parameters.end() && aggIt->second.type == WorkflowParameterValue::Type::String) {
            aggTypeStr = aggIt->second.stringValue;
        }

        const AggregationType aggType = ParseAggregationType(aggTypeStr);

        auto it = globalMetrics_.find(*metricName);
        if (it == globalMetrics_.end()) {
            throw std::runtime_error("debug.metrics no data recorded for metric: " + *metricName);
        }

        const double result = AggregateMetric(it->second, aggType);
        const std::string resultKey = ioResolver.GetRequiredOutputKey(step, "result");
        context.Set(resultKey, result);

        if (logger_) {
            logger_->Trace("WorkflowDebugMetricsStep", "Aggregate",
                          "metric=" + *metricName + ", type=" + aggTypeStr + ", result=" + std::to_string(result),
                          "Aggregated metric");
        }

    } else if (op == Operation::RESET) {
        auto it = globalMetrics_.find(*metricName);
        if (it != globalMetrics_.end()) {
            globalMetrics_.erase(it);
        }

        if (logger_) {
            logger_->Trace("WorkflowDebugMetricsStep", "Reset",
                          "metric=" + *metricName,
                          "Reset metric data");
        }
    }
}

}  // namespace sdl3cpp::services::impl
