#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow_parameter_value.hpp"

#include <memory>

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
 *   - operation: "record" (record a value), "aggregate" (compute stats),
 *     "reset" (clear data)
 *   - agg_type: For aggregate: "min", "max", "avg", "sum", "count"
 *     [default: "avg"]
 *
 * Outputs:
 *   - result: Aggregated result (number) - only on aggregate operation
 *
 * See debug_metrics_store.hpp for the parsing/aggregation/storage helpers.
 */
class WorkflowDebugMetricsStep final : public IWorkflowStep {
public:
    explicit WorkflowDebugMetricsStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
