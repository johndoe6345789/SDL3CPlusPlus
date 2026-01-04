#pragma once

#include "../interfaces/i_logger.hpp"
#include "../../logging/logger.hpp"

namespace sdl3cpp::services::impl {

/**
 * @brief Logger service implementation.
 *
 * Wraps the existing Logger singleton to provide DI-compatible logging.
 * Small, focused service (~30 lines) that adapts the logger for dependency injection.
 */
class LoggerService : public ILogger {
public:
    LoggerService() = default;
    ~LoggerService() override = default;

    // ILogger interface
    void SetLevel(LogLevel level) override {
        logging::Logger::GetInstance().SetLevel(static_cast<logging::LogLevel>(level));
    }

    LogLevel GetLevel() const override {
        return static_cast<LogLevel>(logging::Logger::GetInstance().GetLevel());
    }

    void SetOutputFile(const std::string& filename) override {
        logging::Logger::GetInstance().SetOutputFile(filename);
    }

    void EnableConsoleOutput(bool enable) override {
        logging::Logger::GetInstance().EnableConsoleOutput(enable);
    }

    void Log(LogLevel level, const std::string& message) override {
        logging::Logger::GetInstance().Log(static_cast<logging::LogLevel>(level), message);
    }

    void Trace(const std::string& message) override {
        logging::Logger::GetInstance().Trace(message);
    }

    void Trace(const std::string& className, const std::string& methodName, const std::string& args, const std::string& message) override {
        std::string formattedMessage = className + "::" + methodName;
        if (!args.empty()) {
            formattedMessage += "(" + args + ")";
        }
        if (!message.empty()) {
            formattedMessage += ": " + message;
        }
        logging::Logger::GetInstance().Trace(formattedMessage);
    }

    void Debug(const std::string& message) override {
        logging::Logger::GetInstance().Debug(message);
    }

    void Info(const std::string& message) override {
        logging::Logger::GetInstance().Info(message);
    }

    void Warn(const std::string& message) override {
        logging::Logger::GetInstance().Warn(message);
    }

    void Error(const std::string& message) override {
        logging::Logger::GetInstance().Error(message);
    }

    void TraceFunction(const std::string& funcName) override {
        logging::Logger::GetInstance().TraceFunction(funcName);
    }

    void TraceVariable(const std::string& name, const std::string& value) override {
        logging::Logger::GetInstance().TraceVariable(name, value);
    }

    void TraceVariable(const std::string& name, int value) override {
        logging::Logger::GetInstance().TraceVariable(name, value);
    }

    void TraceVariable(const std::string& name, size_t value) override {
        logging::Logger::GetInstance().TraceVariable(name, value);
    }

    void TraceVariable(const std::string& name, bool value) override {
        logging::Logger::GetInstance().TraceVariable(name, value);
    }

    void TraceVariable(const std::string& name, float value) override {
        logging::Logger::GetInstance().TraceVariable(name, value);
    }

    void TraceVariable(const std::string& name, double value) override {
        logging::Logger::GetInstance().TraceVariable(name, value);
    }
};

} // namespace sdl3cpp::services::impl