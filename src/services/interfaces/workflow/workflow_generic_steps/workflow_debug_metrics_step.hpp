#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow_parameter_value.hpp"

#include <memory>
#include <map>
#include <chrono>

namespace sdl3cpp::services::impl {

/**
 * @brief Workflow step for performance metrics collection
 *
 * Plugin ID: debug.metrics
 * Collects, tracks, and exports performance metrics.
 *
 * Inputs:
 *   - metric_name: Name of the metric to track (string)
 *   - metric_value: Numeric value to record (number)
 *
 * Parameters:
 *   - operation: "record" (record a value), "aggregate" (compute stats), "reset" (clear data)
 *   - agg_type: For aggregate: "min", "max", "avg", "sum", "count" [default: "avg"]
 *
 * Outputs:
 *   - result: Aggregated result (number) - only on aggregate operation
 */
class WorkflowDebugMetricsStep final : public IWorkflowStep {
public:
    explicit WorkflowDebugMetricsStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;

    // Per-step metrics storage (ideally would be persistent across invocations)
    struct MetricData {
        std::vector<double> values;
        size_t recordCount = 0;
        double minValue = 0.0;
        double maxValue = 0.0;
        double sumValue = 0.0;
    };

    // Static map to persist metrics across step invocations
    static std::map<std::string, MetricData> globalMetrics_;

    enum class Operation {
        RECORD,
        AGGREGATE,
        RESET
    };

    enum class AggregationType {
        MIN,
        MAX,
        AVG,
        SUM,
        COUNT
    };

    Operation ParseOperation(const std::string& opStr) const;
    AggregationType ParseAggregationType(const std::string& aggStr) const;
    double AggregateMetric(const MetricData& data, AggregationType type) const;
};

}  // namespace sdl3cpp::services::impl
