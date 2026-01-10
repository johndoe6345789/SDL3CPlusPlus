#pragma once

#include "services/interfaces/i_logger.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace sdl3cpp::services::impl {

// Implementation class that holds all the logging logic
class LoggerImpl {
public:
    static constexpr size_t kDefaultMaxLinesPerFile = 10000;

    std::atomic<LogLevel> level_;
    bool consoleEnabled_;
    std::unique_ptr<std::ofstream> fileStream_;
    std::mutex mutex_;
    std::filesystem::path baseFilePath_;
    size_t maxLinesPerFile_;
    size_t currentLineCount_;
    size_t currentFileIndex_;
    int fileDescriptor_;

    LoggerImpl()
        : level_(LogLevel::INFO),
          consoleEnabled_(true),
          maxLinesPerFile_(kDefaultMaxLinesPerFile),
          currentLineCount_(0),
          currentFileIndex_(0),
          fileDescriptor_(-1) {}

    ~LoggerImpl() {
        if (fileStream_) {
            fileStream_->close();
        }
        CloseFileDescriptor();
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
            SyncFile();
            ++currentLineCount_;
            RotateFileIfNeeded();
        }
    }

    void SetOutputFile(const std::string& filename) {
        if (fileStream_) {
            fileStream_->close();
        }
        fileStream_.reset();
        CloseFileDescriptor();

        baseFilePath_ = filename;
        currentLineCount_ = 0;
        currentFileIndex_ = 0;

        if (!baseFilePath_.empty()) {
            OpenLogFile(currentFileIndex_);
        }
    }

    void SetMaxLinesPerFile(size_t maxLines) {
        maxLinesPerFile_ = maxLines;
    }

    std::filesystem::path BuildLogFilePath(size_t index) const {
        if (baseFilePath_.empty() || index == 0) {
            return baseFilePath_;
        }

        std::filesystem::path basePath(baseFilePath_);
        std::string stem = basePath.stem().string();
        std::string extension = basePath.extension().string();
        std::string rotatedName = stem + "." + std::to_string(index) + extension;

        return basePath.parent_path() / rotatedName;
    }

    void OpenLogFile(size_t index) {
        if (baseFilePath_.empty()) {
            return;
        }

        std::filesystem::path logPath = BuildLogFilePath(index);
        fileStream_ = std::make_unique<std::ofstream>(logPath, std::ios::out | std::ios::trunc);
        if (!fileStream_->is_open()) {
            std::cerr << "Failed to open log file: " << logPath.string() << std::endl;
            fileStream_.reset();
            CloseFileDescriptor();
            return;
        }
        OpenFileDescriptor(logPath);
    }

    void RotateFileIfNeeded() {
        if (maxLinesPerFile_ == 0 || currentLineCount_ < maxLinesPerFile_) {
            return;
        }

        if (fileStream_) {
            fileStream_->close();
        }
        CloseFileDescriptor();

        ++currentFileIndex_;
        currentLineCount_ = 0;
        OpenLogFile(currentFileIndex_);
    }

    void OpenFileDescriptor(const std::filesystem::path& logPath) {
        CloseFileDescriptor();
        const std::string pathString = logPath.string();
#if defined(_WIN32)
        fileDescriptor_ = _open(pathString.c_str(), _O_WRONLY | _O_APPEND);
#else
        fileDescriptor_ = ::open(pathString.c_str(), O_WRONLY | O_APPEND);
#endif
        if (fileDescriptor_ < 0) {
            std::cerr << "Failed to open log file descriptor for sync: " << pathString << std::endl;
        }
    }

    void CloseFileDescriptor() {
        if (fileDescriptor_ < 0) {
            return;
        }
#if defined(_WIN32)
        _close(fileDescriptor_);
#else
        ::close(fileDescriptor_);
#endif
        fileDescriptor_ = -1;
    }

    void SyncFile() {
        if (fileDescriptor_ < 0) {
            return;
        }
#if defined(_WIN32)
        _commit(fileDescriptor_);
#else
        ::fsync(fileDescriptor_);
#endif
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
    void SetMaxLinesPerFile(size_t maxLines) override;
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
