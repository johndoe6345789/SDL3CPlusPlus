#ifndef SDL3CPP_LOGGING_LOGGER_HPP
#define SDL3CPP_LOGGING_LOGGER_HPP

#include <atomic>
#include <iostream>
#include <sstream>
#include <string>
#include <mutex>
#include <fstream>
#include <memory>

namespace sdl3cpp::logging {

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    OFF = 5
};

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
    void Trace(const std::string& message) { Log(LogLevel::TRACE, message); }
    void Debug(const std::string& message) { Log(LogLevel::DEBUG, message); }
    void Info(const std::string& message) { Log(LogLevel::INFO, message); }
    void Warn(const std::string& message) { Log(LogLevel::WARN, message); }
    void Error(const std::string& message) { Log(LogLevel::ERROR, message); }

    // Tracing methods
    void TraceFunction(const std::string& funcName) {
        if (GetLevel() <= LogLevel::TRACE) {
            Trace(std::string("Entering ") + funcName);
        }
    }

    template <typename T>
    void TraceVariable(const std::string& name, const T& value) {
        if (GetLevel() <= LogLevel::TRACE) {
            std::ostringstream oss;
            oss << name << " = " << value;
            Trace(oss.str());
        }
    }

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string LevelToString(LogLevel level) const;
    std::string FormatMessage(LogLevel level, const std::string& message);
    void WriteToConsole(LogLevel level, const std::string& message);
    void WriteToFile(const std::string& message);

    std::atomic<LogLevel> level_;
    bool consoleEnabled_;
    std::unique_ptr<std::ofstream> fileStream_;
    std::mutex mutex_;
};

#define LOG_TRACE(msg) sdl3cpp::logging::Logger::GetInstance().Trace(msg)
#define LOG_DEBUG(msg) sdl3cpp::logging::Logger::GetInstance().Debug(msg)
#define LOG_INFO(msg) sdl3cpp::logging::Logger::GetInstance().Info(msg)
#define LOG_WARN(msg) sdl3cpp::logging::Logger::GetInstance().Warn(msg)
#define LOG_ERROR(msg) sdl3cpp::logging::Logger::GetInstance().Error(msg)

#define TRACE_FUNCTION() sdl3cpp::logging::Logger::GetInstance().TraceFunction(__func__)
#define TRACE_VAR(var) sdl3cpp::logging::Logger::GetInstance().TraceVariable(#var, var)

} // namespace sdl3cpp::logging

#endif // SDL3CPP_LOGGING_LOGGER_HPP