#include "services/interfaces/diagnostics/logger_service.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <cstdlib>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

// Rebuild spdlog logger whenever sinks change (SetOutputFile /
// EnableConsoleOutput).
void LoggerService::RebuildLogger() {
    std::vector<spdlog::sink_ptr> sinks;

    if (consoleOn_) {
        auto s = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        s->set_pattern("[%H:%M:%S.%e] [%^%-5l%$] %v");
        sinks.push_back(s);
    }

    if (!filePath_.empty()) {
        try {
            auto s = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                filePath_, true);
            s->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%-5l] %v");
            sinks.push_back(s);
        } catch (...) {}
    }

    const auto prevLevel = log_ ? log_->level() : spdlog::level::info;
    log_ = std::make_shared<spdlog::logger>("q3", sinks.begin(), sinks.end());
    log_->set_level(prevLevel);
    log_->flush_on(spdlog::level::trace);  // flush all levels immediately
}

// ── construction ──────────────────────────────────────────────

LoggerService::LoggerService() {
    RebuildLogger();

    // Honor QUAKE3_LOG_LEVEL env var at startup:
    // trace/debug/info/warn/error/off
    if (const char* env = std::getenv("QUAKE3_LOG_LEVEL")) {
        const std::string lv(env);
        if (lv == "trace")
            log_->set_level(spdlog::level::trace);
        else if (lv == "debug")
            log_->set_level(spdlog::level::debug);
        else if (lv == "info")
            log_->set_level(spdlog::level::info);
        else if (lv == "warn")
            log_->set_level(spdlog::level::warn);
        else if (lv == "error")
            log_->set_level(spdlog::level::err);
        else if (lv == "off")
            log_->set_level(spdlog::level::off);
    }
}

}  // namespace sdl3cpp::services::impl
