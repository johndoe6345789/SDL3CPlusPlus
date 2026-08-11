#include "services/interfaces/workflow/workflow_generic_steps/workflow_debug_log_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <cctype>
#include <algorithm>

namespace sdl3cpp::services::impl {

WorkflowDebugLogStep::WorkflowDebugLogStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDebugLogStep::GetPluginId() const {
    return "debug.log";
}

WorkflowDebugLogStep::LogLevel WorkflowDebugLogStep::ParseLogLevel(const std::string& levelStr) const {
    std::string normalized = levelStr;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (normalized == "trace") return LogLevel::TRACE;
    if (normalized == "debug") return LogLevel::DEBUG;
    if (normalized == "info") return LogLevel::INFO;
    if (normalized == "warn") return LogLevel::WARN;
    if (normalized == "error") return LogLevel::ERROR;

    return LogLevel::INFO;
}

void WorkflowDebugLogStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver ioResolver;

    const std::string messageKey = ioResolver.GetRequiredInputKey(step, "message");

    const auto* message = context.TryGet<std::string>(messageKey);
    if (!message) {
        throw std::runtime_error("debug.log missing input '" + messageKey + "'");
    }

    // Get level parameter (default: "info")
    std::string level = "info";
    auto it = step.parameters.find("level");
    if (it != step.parameters.end() && it->second.type == WorkflowParameterValue::Type::String) {
        level = it->second.stringValue;
    }

    // Get context parameter (default: "debug.log")
    std::string contextLabel = "debug.log";
    auto ctxIt = step.parameters.find("context");
    if (ctxIt != step.parameters.end() && ctxIt->second.type == WorkflowParameterValue::Type::String) {
        contextLabel = ctxIt->second.stringValue;
    }

    const LogLevel logLevel = ParseLogLevel(level);

    if (!logger_) {
        return;
    }

    switch (logLevel) {
        case LogLevel::TRACE:
            logger_->Trace(contextLabel, "Debug Log", "message=" + *message, "Logged trace message");
            break;
        case LogLevel::DEBUG:
            logger_->Debug("debug.log: " + *message);
            break;
        case LogLevel::INFO:
            logger_->Info("debug.log: " + *message);
            break;
        case LogLevel::WARN:
            logger_->Warn("debug.log: " + *message);
            break;
        case LogLevel::ERROR:
            logger_->Error("debug.log: " + *message);
            break;
    }
}

}  // namespace sdl3cpp::services::impl
