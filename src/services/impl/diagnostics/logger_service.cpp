#include "services/interfaces/diagnostics/logger_service.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <cstdlib>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

// ── helpers ───────────────────────────────────────────────────────────────────

spdlog::level::level_enum LoggerService::ToSpdlog(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return spdlog::level::trace;
        case LogLevel::DEBUG: return spdlog::level::debug;
        case LogLevel::INFO:  return spdlog::level::info;
        case LogLevel::WARN:  return spdlog::level::warn;
        case LogLevel::ERROR: return spdlog::level::err;
        default:              return spdlog::level::off;
    }
}

LogLevel LoggerService::FromSpdlog(spdlog::level::level_enum l) {
    switch (l) {
        case spdlog::level::trace:    return LogLevel::TRACE;
        case spdlog::level::debug:    return LogLevel::DEBUG;
        case spdlog::level::info:     return LogLevel::INFO;
        case spdlog::level::warn:     return LogLevel::WARN;
        case spdlog::level::err:
        case spdlog::level::critical: return LogLevel::ERROR;
        default:                      return LogLevel::OFF;
    }
}

// Rebuild spdlog logger whenever sinks change (SetOutputFile / EnableConsoleOutput).
void LoggerService::RebuildLogger() {
    std::vector<spdlog::sink_ptr> sinks;

    if (consoleOn_) {
        auto s = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        s->set_pattern("[%H:%M:%S.%e] [%^%-5l%$] %v");
        sinks.push_back(s);
    }

    if (!filePath_.empty()) {
        try {
            auto s = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filePath_, true);
            s->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%-5l] %v");
            sinks.push_back(s);
        } catch (...) {}
    }

    const auto prevLevel = log_ ? log_->level() : spdlog::level::info;
    log_ = std::make_shared<spdlog::logger>("q3", sinks.begin(), sinks.end());
    log_->set_level(prevLevel);
    log_->flush_on(spdlog::level::trace);  // flush all levels immediately
}

// ── construction ──────────────────────────────────────────────────────────────

LoggerService::LoggerService() {
    RebuildLogger();

    // Honor QUAKE3_LOG_LEVEL env var at startup: trace/debug/info/warn/error/off
    if (const char* env = std::getenv("QUAKE3_LOG_LEVEL")) {
        const std::string lv(env);
        if      (lv == "trace") log_->set_level(spdlog::level::trace);
        else if (lv == "debug") log_->set_level(spdlog::level::debug);
        else if (lv == "info")  log_->set_level(spdlog::level::info);
        else if (lv == "warn")  log_->set_level(spdlog::level::warn);
        else if (lv == "error") log_->set_level(spdlog::level::err);
        else if (lv == "off")   log_->set_level(spdlog::level::off);
    }
}

// ── ILogger ───────────────────────────────────────────────────────────────────

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

void LoggerService::Log(LogLevel level, const std::string& message) {
    log_->log(ToSpdlog(level), message);
}

void LoggerService::Trace(const std::string& message) {
    log_->trace(message);
}

void LoggerService::Trace(const std::string& className, const std::string& methodName,
                          const std::string& args, const std::string& message) {
    if (!log_->should_log(spdlog::level::trace)) return;
    std::string s = className + "::" + methodName;
    if (!args.empty())    s += "(" + args + ")";
    if (!message.empty()) s += " — " + message;
    log_->trace(s);
}

void LoggerService::Debug(const std::string& message) { log_->debug(message); }
void LoggerService::Info (const std::string& message) { log_->info (message); }
void LoggerService::Warn (const std::string& message) { log_->warn (message); }
void LoggerService::Error(const std::string& message) { log_->error(message); }

void LoggerService::TraceFunction(const std::string& funcName) {
    log_->trace("→ {}", funcName);
}

void LoggerService::TraceVariable(const std::string& name, const std::string& value) {
    log_->trace("  {} = {}", name, value);
}
void LoggerService::TraceVariable(const std::string& name, int    v) { log_->trace("  {} = {}", name, v); }
void LoggerService::TraceVariable(const std::string& name, size_t v) { log_->trace("  {} = {}", name, v); }
void LoggerService::TraceVariable(const std::string& name, bool   v) { log_->trace("  {} = {}", name, v ? "true" : "false"); }
void LoggerService::TraceVariable(const std::string& name, float  v) { log_->trace("  {} = {:.4f}", name, v); }
void LoggerService::TraceVariable(const std::string& name, double v) { log_->trace("  {} = {:.6f}", name, v); }

} // namespace sdl3cpp::services::impl
