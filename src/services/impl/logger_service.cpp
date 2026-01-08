#include "logger_service.hpp"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <thread>

namespace sdl3cpp::services::impl {

LoggerService::LoggerService() : impl_(std::make_unique<LoggerImpl>()) {}

void LoggerService::SetLevel(LogLevel level) {
    impl_->level_.store(level, std::memory_order_relaxed);
}

LogLevel LoggerService::GetLevel() const {
    return impl_->level_.load(std::memory_order_relaxed);
}

void LoggerService::SetOutputFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->SetOutputFile(filename);
}

void LoggerService::SetMaxLinesPerFile(size_t maxLines) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->SetMaxLinesPerFile(maxLines);
}

void LoggerService::EnableConsoleOutput(bool enable) {
    impl_->consoleEnabled_ = enable;
}

void LoggerService::Log(LogLevel level, const std::string& message) {
    if (level < GetLevel()) {
        return;
    }

    std::lock_guard<std::mutex> lock(impl_->mutex_);
    std::string formattedMessage = impl_->FormatMessage(level, message);

    if (impl_->consoleEnabled_) {
        impl_->WriteToConsole(level, formattedMessage);
    }

    if (impl_->fileStream_) {
        impl_->WriteToFile(formattedMessage);
    }
}

void LoggerService::Trace(const std::string& message) {
    Log(LogLevel::TRACE, message);
}

void LoggerService::Trace(const std::string& className, const std::string& methodName, const std::string& args, const std::string& message) {
    std::string formattedMessage = className + "::" + methodName;
    if (!args.empty()) {
        formattedMessage += "(" + args + ")";
    }
    if (!message.empty()) {
        formattedMessage += ": " + message;
    }
    Log(LogLevel::TRACE, formattedMessage);
}

void LoggerService::Debug(const std::string& message) {
    Log(LogLevel::DEBUG, message);
}

void LoggerService::Info(const std::string& message) {
    Log(LogLevel::INFO, message);
}

void LoggerService::Warn(const std::string& message) {
    Log(LogLevel::WARN, message);
}

void LoggerService::Error(const std::string& message) {
    Log(LogLevel::ERROR, message);
}

void LoggerService::TraceFunction(const std::string& funcName) {
    if (GetLevel() <= LogLevel::TRACE) {
        Trace("Entering " + funcName);
    }
}

void LoggerService::TraceVariable(const std::string& name, const std::string& value) {
    if (GetLevel() <= LogLevel::TRACE) {
        Trace(name + " = " + value);
    }
}

void LoggerService::TraceVariable(const std::string& name, int value) {
    TraceVariable(name, std::to_string(value));
}

void LoggerService::TraceVariable(const std::string& name, size_t value) {
    TraceVariable(name, std::to_string(value));
}

void LoggerService::TraceVariable(const std::string& name, bool value) {
    TraceVariable(name, std::string(value ? "true" : "false"));
}

void LoggerService::TraceVariable(const std::string& name, float value) {
    TraceVariable(name, std::to_string(value));
}

void LoggerService::TraceVariable(const std::string& name, double value) {
    TraceVariable(name, std::to_string(value));
}

} // namespace sdl3cpp::services::impl
