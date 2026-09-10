#include "services/interfaces/workflow/workflow_generic_steps/debug_log_helpers.hpp"
#include "services/interfaces/workflow_parameter_value.hpp"

#include <algorithm>
#include <cctype>

namespace sdl3cpp::services::impl {

DebugLogLevel ParseDebugLogLevel(const std::string& levelStr) {
    std::string normalized = levelStr;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (normalized == "trace") return DebugLogLevel::TRACE;
    if (normalized == "debug") return DebugLogLevel::DEBUG;
    if (normalized == "info") return DebugLogLevel::INFO;
    if (normalized == "warn") return DebugLogLevel::WARN;
    if (normalized == "error") return DebugLogLevel::ERROR;
    return DebugLogLevel::INFO;
}

DebugLogParams ReadDebugLogParams(const WorkflowStepDefinition& step) {
    DebugLogParams params;

    auto it = step.parameters.find("level");
    if (it != step.parameters.end() &&
        it->second.type == WorkflowParameterValue::Type::String) {
        params.level = it->second.stringValue;
    }

    auto ctxIt = step.parameters.find("context");
    if (ctxIt != step.parameters.end() &&
        ctxIt->second.type == WorkflowParameterValue::Type::String) {
        params.contextLabel = ctxIt->second.stringValue;
    }
    return params;
}

void EmitDebugLog(const std::shared_ptr<ILogger>& logger, DebugLogLevel level,
                  const std::string& contextLabel, const std::string& message) {
    if (!logger) return;

    switch (level) {
        case DebugLogLevel::TRACE:
            logger->Trace(contextLabel, "Debug Log", "message=" + message,
                          "Logged trace message");
            break;
        case DebugLogLevel::DEBUG:
            logger->Debug("debug.log: " + message);
            break;
        case DebugLogLevel::INFO:
            logger->Info("debug.log: " + message);
            break;
        case DebugLogLevel::WARN:
            logger->Warn("debug.log: " + message);
            break;
        case DebugLogLevel::ERROR:
            logger->Error("debug.log: " + message);
            break;
    }
}

}  // namespace sdl3cpp::services::impl
