#pragma once

#include "services/interfaces/i_logger.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/logger.h>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief spdlog-backed ILogger implementation.
 *
 * Console sink always active; file sink added on SetOutputFile().
 * Log level controlled at runtime via QUAKE3_LOG_LEVEL env var
 * (trace / debug / info / warn / error) or programmatically via SetLevel().
 */
class LoggerService : public ILogger {
public:
    LoggerService();
    ~LoggerService() override = default;

    void SetLevel(LogLevel level) override;
    LogLevel GetLevel() const override;
    void SetOutputFile(const std::string& filename) override;
    void SetMaxLinesPerFile(size_t maxLines) override;  // no-op: spdlog handles rotation
    void EnableConsoleOutput(bool enable) override;

    void Log(LogLevel level, const std::string& message) override;
    void Trace(const std::string& message) override;
    void Trace(const std::string& className, const std::string& methodName,
               const std::string& args = "", const std::string& message = "") override;
    void Debug(const std::string& message) override;
    void Info(const std::string& message) override;
    void Warn(const std::string& message) override;
    void Error(const std::string& message) override;

    void TraceFunction(const std::string& funcName) override;
    void TraceVariable(const std::string& name, const std::string& value) override;
    void TraceVariable(const std::string& name, int value) override;
    void TraceVariable(const std::string& name, size_t value) override;
    void TraceVariable(const std::string& name, bool value) override;
    void TraceVariable(const std::string& name, float value) override;
    void TraceVariable(const std::string& name, double value) override;

private:
    static spdlog::level::level_enum ToSpdlog(LogLevel level);
    static LogLevel                  FromSpdlog(spdlog::level::level_enum l);
    void RebuildLogger();

    std::string filePath_;
    bool        consoleOn_ = true;
    std::shared_ptr<spdlog::logger> log_;
};

} // namespace sdl3cpp::services::impl
