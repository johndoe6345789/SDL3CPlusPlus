#include "services/interfaces/diagnostics/logger_service.hpp"

namespace sdl3cpp::services::impl {

// ── level conversion ─────────────────────────────────────────

spdlog::level::level_enum LoggerService::ToSpdlog(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:
            return spdlog::level::trace;
        case LogLevel::DEBUG:
            return spdlog::level::debug;
        case LogLevel::INFO:
            return spdlog::level::info;
        case LogLevel::WARN:
            return spdlog::level::warn;
        case LogLevel::ERROR:
            return spdlog::level::err;
        default:
            return spdlog::level::off;
    }
}

LogLevel LoggerService::FromSpdlog(spdlog::level::level_enum l) {
    switch (l) {
        case spdlog::level::trace:
            return LogLevel::TRACE;
        case spdlog::level::debug:
            return LogLevel::DEBUG;
        case spdlog::level::info:
            return LogLevel::INFO;
        case spdlog::level::warn:
            return LogLevel::WARN;
        case spdlog::level::err:
        case spdlog::level::critical:
            return LogLevel::ERROR;
        default:
            return LogLevel::OFF;
    }
}

}  // namespace sdl3cpp::services::impl
