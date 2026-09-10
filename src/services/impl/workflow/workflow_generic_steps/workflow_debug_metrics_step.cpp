#include "services/interfaces/workflow/workflow_generic_steps/workflow_debug_metrics_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/metric_store.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowDebugMetricsStep::WorkflowDebugMetricsStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDebugMetricsStep::GetPluginId() const {
    return "debug.metrics";
}

void WorkflowDebugMetricsStep::Execute(const WorkflowStepDefinition& step,
                                       WorkflowContext& context) {
    WorkflowStepIoResolver io;
    std::string metricNameKey = io.GetRequiredInputKey(step, "metric_name");
    const auto* metricName    = context.TryGet<std::string>(metricNameKey);
    if (!metricName) {
        throw std::runtime_error("debug.metrics missing input '" +
                                 metricNameKey + "'");
    }
    auto& store             = DebugMetricsStore();
    const std::string opStr = FindStringParam(step, "operation", "record");
    const DebugMetricOperation op = ParseDebugMetricOperation(opStr);
    const auto trace = [&](const char* method, const std::string& detail,
                           const char* msg) {
        if (logger_) {
            logger_->Trace("WorkflowDebugMetricsStep", method, detail, msg);
        }
    };

    if (op == DebugMetricOperation::RECORD) {
        std::string valueKey = io.GetRequiredInputKey(step, "metric_value");
        const auto* value    = context.TryGet<double>(valueKey);
        if (!value) {
            throw std::runtime_error("debug.metrics missing input '" +
                                     valueKey + "' for record operation");
        }
        RecordDebugMetric(store[*metricName], *value);
        trace("Record",
              "metric=" + *metricName + ", value=" + std::to_string(*value),
              "Recorded metric value");
    } else if (op == DebugMetricOperation::AGGREGATE) {
        const std::string aggStr = FindStringParam(step, "agg_type", "avg");
        const DebugMetricAggregation aggType =
            ParseDebugMetricAggregation(aggStr);
        auto it = store.find(*metricName);
        if (it == store.end()) {
            throw std::runtime_error(
                "debug.metrics no data recorded for metric: " + *metricName);
        }
        const double result = AggregateDebugMetric(it->second, aggType);
        context.Set(io.GetRequiredOutputKey(step, "result"), result);
        trace("Aggregate",
              "metric=" + *metricName + ", type=" + aggStr +
                  ", result=" + std::to_string(result),
              "Aggregated metric");
    } else if (op == DebugMetricOperation::RESET) {
        store.erase(*metricName);
        trace("Reset", "metric=" + *metricName, "Reset metric data");
    }
}

}  // namespace sdl3cpp::services::impl
