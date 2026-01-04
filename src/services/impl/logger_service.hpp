#pragma once

#include "../interfaces/i_logger.hpp"
#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

namespace sdl3cpp::services::impl {

// Implementation class that holds all the logging logic
class LoggerImpl {
public:
    std::atomic<LogLevel> level_;
    bool consoleEnabled_;
    std::unique_ptr<std::ofstream> fileStream_;
    std::mutex mutex_;

    LoggerImpl() : level_(LogLevel::INFO), consoleEnabled_(true) {}

    ~LoggerImpl() {
        if (fileStream_) {
            fileStream_->close();
        }
    }

    std::string LevelToString(LogLevel level) const {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARN: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    std::string FormatMessage(LogLevel level, const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << ms.count()
            << " [" << LevelToString(level) << "] "
            << message;
        return oss.str();
    }

    void WriteToConsole(LogLevel level, const std::string& message) {
        if (level >= LogLevel::ERROR) {
            std::cerr << message << std::endl;
        } else {
            std::cout << message << std::endl;
        }
    }

    void WriteToFile(const std::string& message) {
        if (fileStream_) {
            *fileStream_ << message << std::endl;
            fileStream_->flush();
        }
    }
};

/**
 * @brief Logger service implementation.
 *
 * Contains the full logging implementation, no longer wrapping a singleton.
 * Small, focused service (~200 lines) for application logging.
 */
class LoggerService : public ILogger {
public:
    LoggerService();
    ~LoggerService() override = default;

    // ILogger interface
    void SetLevel(LogLevel level) override;
    LogLevel GetLevel() const override;
    void SetOutputFile(const std::string& filename) override;
    void EnableConsoleOutput(bool enable) override;
    void Log(LogLevel level, const std::string& message) override;
    void Trace(const std::string& message) override;
    void Trace(const std::string& className, const std::string& methodName, const std::string& args = "", const std::string& message = "") override;
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
    std::unique_ptr<LoggerImpl> impl_;
};

} // namespace sdl3cpp::services::impl