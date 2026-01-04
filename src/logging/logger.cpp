#include "logging/logger.hpp"

#include <chrono>
#include <iomanip>
#include <thread>

namespace sdl3cpp::logging {

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : level_(LogLevel::INFO), consoleEnabled_(true) {}

Logger::~Logger() {
    if (fileStream_) {
        fileStream_->close();
    }
}

void Logger::SetLevel(LogLevel level) {
    level_.store(level, std::memory_order_relaxed);
}

LogLevel Logger::GetLevel() const {
    return level_.load(std::memory_order_relaxed);
}

void Logger::SetOutputFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fileStream_) {
        fileStream_->close();
    }
    fileStream_ = std::make_unique<std::ofstream>(filename, std::ios::app);
    if (!fileStream_->is_open()) {
        // Fallback to console if file can't be opened
        std::cerr << "Failed to open log file: " << filename << std::endl;
        fileStream_.reset();
    }
}

void Logger::EnableConsoleOutput(bool enable) {
    consoleEnabled_ = enable;
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (level < GetLevel()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::string formattedMessage = FormatMessage(level, message);

    if (consoleEnabled_) {
        WriteToConsole(level, formattedMessage);
    }

    if (fileStream_) {
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
    if (fileStream_) {
        *fileStream_ << message << std::endl;
        fileStream_->flush();
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

} // namespace sdl3cpp::logging