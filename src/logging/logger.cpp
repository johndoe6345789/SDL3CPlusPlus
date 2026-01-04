#include "logging/logger.hpp"

#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

namespace sdl3cpp::logging {

// Implementation class that holds all the C++ magic
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
};

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : impl_(new LoggerImpl()) {}

Logger::~Logger() {
    delete impl_;
}

void Logger::SetLevel(LogLevel level) {
    impl_->level_.store(level, std::memory_order_relaxed);
}

LogLevel Logger::GetLevel() const {
    return impl_->level_.load(std::memory_order_relaxed);
}

void Logger::SetOutputFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    if (impl_->fileStream_) {
        impl_->fileStream_->close();
    }
    impl_->fileStream_ = std::make_unique<std::ofstream>(filename, std::ios::app);
    if (!impl_->fileStream_->is_open()) {
        // Fallback to console if file can't be opened
        std::cerr << "Failed to open log file: " << filename << std::endl;
        impl_->fileStream_.reset();
    }
}

void Logger::EnableConsoleOutput(bool enable) {
    impl_->consoleEnabled_ = enable;
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (level < GetLevel()) {
        return;
    }

    std::lock_guard<std::mutex> lock(impl_->mutex_);
    std::string formattedMessage = FormatMessage(level, message);

    if (impl_->consoleEnabled_) {
        WriteToConsole(level, formattedMessage);
    }

    if (impl_->fileStream_) {
        WriteToFile(formattedMessage);
    }
}

void Logger::Log(LogLevel level, const char* message) {
    Log(level, std::string(message));
}

std::string Logger::LevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void Logger::WriteToConsole(LogLevel level, const std::string& message) {
    if (level >= LogLevel::ERROR) {
        std::cerr << message << std::endl;
    } else {
        std::cout << message << std::endl;
    }
}

void Logger::WriteToFile(const std::string& message) {
    if (impl_->fileStream_) {
        *impl_->fileStream_ << message << std::endl;
        impl_->fileStream_->flush();
    }
}

std::string Logger::FormatMessage(LogLevel level, const std::string& message) {
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

void Logger::Trace(const std::string& message) {
    Log(LogLevel::TRACE, message);
}

void Logger::Debug(const std::string& message) {
    Log(LogLevel::DEBUG, message);
}

void Logger::Info(const std::string& message) {
    Log(LogLevel::INFO, message);
}

void Logger::Warn(const std::string& message) {
    Log(LogLevel::WARN, message);
}

void Logger::Error(const std::string& message) {
    Log(LogLevel::ERROR, message);
}

void Logger::TraceFunction(const std::string& funcName) {
    if (GetLevel() <= LogLevel::TRACE) {
        Trace("Entering " + funcName);
    }
}

void Logger::TraceVariable(const std::string& name, const std::string& value) {
    if (GetLevel() <= LogLevel::TRACE) {
        Trace(name + " = " + value);
    }
}

void Logger::TraceVariable(const std::string& name, int value) {
    TraceVariable(name, std::to_string(value));
}

void Logger::TraceVariable(const std::string& name, size_t value) {
    TraceVariable(name, std::to_string(value));
}

void Logger::TraceVariable(const std::string& name, bool value) {
    TraceVariable(name, value ? "true" : "false");
}

void Logger::TraceVariable(const std::string& name, float value) {
    TraceVariable(name, std::to_string(value));
}

void Logger::TraceVariable(const std::string& name, double value) {
    TraceVariable(name, std::to_string(value));
}

void Logger::TraceFunctionWithArgs(const std::string& description, const std::string& args) {
    if (GetLevel() <= LogLevel::TRACE) {
        Trace(description + ": " + args);
    }
}

TraceGuard::TraceGuard(const std::string& funcName)
    : funcName_(funcName), ended_(false) {
    if (!funcName_.empty()) {
        Logger::GetInstance().Trace("Entering " + funcName_);
    }
}

void TraceGuard::End() {
    if (!ended_ && !funcName_.empty()) {
        Logger::GetInstance().Trace("Exiting " + funcName_);
        ended_ = true;
    }
}

} // namespace sdl3cpp::logging