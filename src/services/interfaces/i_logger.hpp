#pragma once

#include <string>

namespace sdl3cpp::services {

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    OFF = 5
};

/**
 * @brief Logger service interface.
 *
 * Provides logging functionality with different log levels.
 * Small, focused service (~50 lines) for application logging.
 */
class ILogger {
public:
    virtual ~ILogger() = default;

    /**
     * @brief Set the logging level.
     *
     * @param level The minimum level to log
     */
    virtual void SetLevel(LogLevel level) = 0;

    /**
     * @brief Get the current logging level.
     *
     * @return Current log level
     */
    virtual LogLevel GetLevel() const = 0;

    /**
     * @brief Set the output file for logging.
     *
     * @param filename Path to the log file
     */
    virtual void SetOutputFile(const std::string& filename) = 0;

    /**
     * @brief Set the maximum number of log lines per file before rotation.
     *
     * @param maxLines Max lines per log file, or 0 to disable rotation
     */
    virtual void SetMaxLinesPerFile(size_t maxLines) = 0;

    /**
     * @brief Enable or disable console output.
     *
     * @param enable True to enable console output
     */
    virtual void EnableConsoleOutput(bool enable) = 0;

    /**
     * @brief Log a message with the specified level.
     *
     * @param level Log level
     * @param message Log message
     */
    virtual void Log(LogLevel level, const std::string& message) = 0;

    /**
     * @brief Log a trace message.
     *
     * @param message Trace message
     */
    virtual void Trace(const std::string& message) = 0;

    /**
     * @brief Log a trace message with class, method, and arguments information.
     *
     * @param className Name of the class
     * @param methodName Name of the method
     * @param args Arguments passed to the method (as formatted string)
     * @param message Optional additional message
     */
    virtual void Trace(const std::string& className, const std::string& methodName, const std::string& args = "", const std::string& message = "") = 0;

    /**
     * @brief Log a debug message.
     *
     * @param message Debug message
     */
    virtual void Debug(const std::string& message) = 0;

    /**
     * @brief Log an info message.
     *
     * @param message Info message
     */
    virtual void Info(const std::string& message) = 0;

    /**
     * @brief Log a warning message.
     *
     * @param message Warning message
     */
    virtual void Warn(const std::string& message) = 0;

    /**
     * @brief Log an error message.
     *
     * @param message Error message
     */
    virtual void Error(const std::string& message) = 0;

    /**
     * @brief Log function entry.
     *
     * @param funcName Function name
     */
    virtual void TraceFunction(const std::string& funcName) = 0;

    /**
     * @brief Trace a variable value.
     *
     * @param name Variable name
     * @param value Variable value
     */
    virtual void TraceVariable(const std::string& name, const std::string& value) = 0;
    virtual void TraceVariable(const std::string& name, int value) = 0;
    virtual void TraceVariable(const std::string& name, size_t value) = 0;
    virtual void TraceVariable(const std::string& name, bool value) = 0;
    virtual void TraceVariable(const std::string& name, float value) = 0;
    virtual void TraceVariable(const std::string& name, double value) = 0;
};

} // namespace sdl3cpp::services
