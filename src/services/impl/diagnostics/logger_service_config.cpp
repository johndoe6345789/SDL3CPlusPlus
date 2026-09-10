#include "services/interfaces/diagnostics/logger_service.hpp"

namespace sdl3cpp::services::impl {

// ── config ───────────────────────────────────────────────────

void LoggerService::SetLevel(LogLevel level) {
    log_->set_level(ToSpdlog(level));
}

LogLevel LoggerService::GetLevel() const {
    return FromSpdlog(log_->level());
}

void LoggerService::SetOutputFile(const std::string& filename) {
    filePath_ = filename;
    RebuildLogger();
}

void LoggerService::SetMaxLinesPerFile(size_t /*maxLines*/) {
    // Extend to spdlog::sinks::rotating_file_sink_mt if rotation is needed.
}

void LoggerService::EnableConsoleOutput(bool enable) {
    consoleOn_ = enable;
    RebuildLogger();
}

} // namespace sdl3cpp::services::impl
