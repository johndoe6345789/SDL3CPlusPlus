#ifndef SDL3CPP_LOGGING_LOGGER_HPP
#define SDL3CPP_LOGGING_LOGGER_HPP

#include <string>

namespace sdl3cpp::logging {

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    OFF = 5
};

// Forward declaration to hide implementation details
class LoggerImpl;

class Logger {
public:
    static Logger& GetInstance();

    void SetLevel(LogLevel level);
    LogLevel GetLevel() const;

    void SetOutputFile(const std::string& filename);
    void EnableConsoleOutput(bool enable);

    void Log(LogLevel level, const std::string& message);
    void Log(LogLevel level, const char* message);

    // Convenience methods
    void Trace(const std::string& message);
    void Debug(const std::string& message);
    void Info(const std::string& message);
    void Warn(const std::string& message);
    void Error(const std::string& message);

    // Tracing methods
    void TraceFunction(const std::string& funcName);

    // TraceVariable overloads for common types
    void TraceVariable(const std::string& name, const std::string& value);
    void TraceVariable(const std::string& name, int value);
    void TraceVariable(const std::string& name, size_t value);
    void TraceVariable(const std::string& name, bool value);
    void TraceVariable(const std::string& name, float value);
    void TraceVariable(const std::string& name, double value);

    void TraceFunctionWithArgs(const std::string& description, const std::string& args);

private:
    Logger();
    ~Logger();

    // Non-copyable (Java final class pattern)
    Logger(const Logger&);
    Logger& operator=(const Logger&);

    std::string LevelToString(LogLevel level) const;
    std::string FormatMessage(LogLevel level, const std::string& message);
    void WriteToConsole(LogLevel level, const std::string& message);
    void WriteToFile(const std::string& message);

    LoggerImpl* impl_;
};

class TraceGuard {
public:
    explicit TraceGuard(const std::string& funcName = "");
    void End();
private:
    std::string funcName_;
    bool ended_;
};

} // namespace sdl3cpp::logging

#endif // SDL3CPP_LOGGING_LOGGER_HPP