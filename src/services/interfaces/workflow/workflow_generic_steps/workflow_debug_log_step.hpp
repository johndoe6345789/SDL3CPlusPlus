#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow_parameter_value.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Workflow step for debug logging
 *
 * Plugin ID: debug.log
 * Logs messages at specified severity level with context information.
 *
 * Inputs:
 *   - message: The message to log (string)
 *
 * Parameters:
 *   - level: Log level (trace, debug, info, warn, error) [default: info]
 *   - context: Optional context label (string) [default: "debug.log"]
 *
 * Outputs:
 *   - None
 */
class WorkflowDebugLogStep final : public IWorkflowStep {
public:
    explicit WorkflowDebugLogStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;

    enum class LogLevel {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR
    };

    LogLevel ParseLogLevel(const std::string& levelStr) const;
};

}  // namespace sdl3cpp::services::impl
