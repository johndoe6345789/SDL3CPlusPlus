#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

enum class DebugLogLevel { TRACE, DEBUG, INFO, WARN, ERROR };

/// Case-insensitive parse of "trace"/"debug"/"info"/"warn"/"error";
/// anything else (including unrecognized text) defaults to INFO.
DebugLogLevel ParseDebugLogLevel(const std::string& levelStr);

/// debug.log's `level` (default "info") and `context` (default
/// "debug.log") parameters.
struct DebugLogParams {
    std::string level        = "info";
    std::string contextLabel = "debug.log";
};
DebugLogParams ReadDebugLogParams(const WorkflowStepDefinition& step);

/// Logs `message` at `level` via `logger` (a no-op if null), using
/// `contextLabel` for the TRACE variant only — the other levels prefix
/// "debug.log: ", exactly as debug.log always has.
void EmitDebugLog(const std::shared_ptr<ILogger>& logger, DebugLogLevel level,
                  const std::string& contextLabel, const std::string& message);

}  // namespace sdl3cpp::services::impl
