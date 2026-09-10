#include "services/interfaces/diagnostics/logger_service.hpp"

namespace sdl3cpp::services::impl {

// ── trace helpers ────────────────────────────────────────────

void LoggerService::TraceFunction(const std::string& funcName) {
    log_->trace("→ {}", funcName);
}

void LoggerService::TraceVariable(const std::string& name,
                                  const std::string& value) {
    log_->trace("  {} = {}", name, value);
}
void LoggerService::TraceVariable(const std::string& name, int v) {
    log_->trace("  {} = {}", name, v);
}
void LoggerService::TraceVariable(const std::string& name, size_t v) {
    log_->trace("  {} = {}", name, v);
}
void LoggerService::TraceVariable(const std::string& name, bool v) {
    log_->trace("  {} = {}", name, v ? "true" : "false");
}
void LoggerService::TraceVariable(const std::string& name, float v) {
    log_->trace("  {} = {:.4f}", name, v);
}
void LoggerService::TraceVariable(const std::string& name, double v) {
    log_->trace("  {} = {:.6f}", name, v);
}

}  // namespace sdl3cpp::services::impl
