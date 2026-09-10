#include "services/interfaces/diagnostics/logger_service.hpp"

namespace sdl3cpp::services::impl {

// ── ILogger ──────────────────────────────────────────────────

void LoggerService::Log(LogLevel level, const std::string& message) {
    log_->log(ToSpdlog(level), message);
}

void LoggerService::Trace(const std::string& message) {
    log_->trace(message);
}

void LoggerService::Trace(const std::string& className,
                          const std::string& methodName,
                          const std::string& args, const std::string& message) {
    if (!log_->should_log(spdlog::level::trace)) return;
    std::string s = className + "::" + methodName;
    if (!args.empty()) s += "(" + args + ")";
    if (!message.empty()) s += " — " + message;
    log_->trace(s);
}

void LoggerService::Debug(const std::string& message) {
    log_->debug(message);
}
void LoggerService::Info(const std::string& message) {
    log_->info(message);
}
void LoggerService::Warn(const std::string& message) {
    log_->warn(message);
}
void LoggerService::Error(const std::string& message) {
    log_->error(message);
}

}  // namespace sdl3cpp::services::impl
