#include "crash_recovery_service.hpp"
#include <exception>
#include <future>
#include <chrono>
#include <sstream>
#include <cstring>
#include <cpptrace/cpptrace.hpp>
#include <unistd.h>
#include <filesystem>
#include <sys/resource.h>

namespace sdl3cpp::services::impl {

namespace {
constexpr int kSignalExitCodeBase = 128;
constexpr size_t kStackTraceSkipFrames = 3;
constexpr size_t kStackTraceMaxDepth = 64;
volatile sig_atomic_t crashHandlingInProgress = 0;

std::string BuildStackTrace() {
    const auto trace = cpptrace::generate_trace(kStackTraceSkipFrames, kStackTraceMaxDepth);
    if (trace.empty()) {
        return {};
    }
    return trace.to_string();
}

int64_t GetSteadyClockNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
}

// Static instance for signal handler
CrashRecoveryService* CrashRecoveryService::instance_ = nullptr;

CrashRecoveryService::CrashRecoveryService(std::shared_ptr<ILogger> logger, CrashRecoveryConfig config)
    : logger_(std::move(logger))
    , crashDetected_(false)
    , lastSignal_(0)
    , lastHeartbeatNs_(0)
    , heartbeatSeen_(false)
    , heartbeatMonitorRunning_(false)
    , config_(std::move(config))
    , memoryLimitBytes_(0)
    , lastSuccessfulFrameTime_(0.0)
    , consecutiveFrameTimeouts_(0)
    , fileFormatErrors_(0)
    , memoryWarnings_(0)
    , lastHealthCheck_(std::chrono::steady_clock::now())
    , signalHandlersInstalled_(false) {
    heartbeatTimeout_ = std::chrono::milliseconds(config_.heartbeatTimeoutMs);
    heartbeatPollInterval_ = std::chrono::milliseconds(config_.heartbeatPollIntervalMs);
    memoryLimitBytes_ = config_.memoryLimitMB * 1024 * 1024;

    if (logger_) {
        logger_->Trace("CrashRecoveryService", "CrashRecoveryService",
                       "logger=" + std::string(logger_ ? "set" : "null") +
                       ", heartbeatTimeoutMs=" + std::to_string(config_.heartbeatTimeoutMs) +
                       ", heartbeatPollIntervalMs=" + std::to_string(config_.heartbeatPollIntervalMs) +
                       ", memoryLimitMB=" + std::to_string(config_.memoryLimitMB));
    }
}

CrashRecoveryService::~CrashRecoveryService() {
    logger_->Trace("CrashRecoveryService", "~CrashRecoveryService", "", "Destroying");
    Shutdown();
}

void CrashRecoveryService::Initialize() {
    logger_->Trace("CrashRecoveryService", "Initialize", "", "Initializing crash recovery service");

    SetupSignalHandlers();
    crashDetected_ = false;
    lastSignal_ = 0;
    lastHeartbeatNs_ = 0;
    heartbeatSeen_ = false;
    crashReport_.clear();

    const bool heartbeatEnabled = config_.heartbeatTimeoutMs > 0 && config_.heartbeatPollIntervalMs > 0;
    if (heartbeatEnabled) {
        if (!heartbeatMonitorRunning_.exchange(true)) {
            heartbeatMonitorThread_ = std::thread(&CrashRecoveryService::MonitorHeartbeats, this);
        }
    } else {
        heartbeatMonitorRunning_ = false;
        if (logger_) {
            logger_->Trace("CrashRecoveryService", "Initialize",
                           "heartbeatEnabled=false", "Heartbeat monitor disabled via config");
        }
    }

    logger_->Info("CrashRecoveryService::Initialize: Crash recovery service initialized");
}

void CrashRecoveryService::Shutdown() {
    logger_->Trace("CrashRecoveryService", "Shutdown", "", "Shutting down crash recovery service");

    heartbeatMonitorRunning_ = false;
    if (heartbeatMonitorThread_.joinable()) {
        heartbeatMonitorThread_.join();
    }

    RemoveSignalHandlers();

    logger_->Info("CrashRecoveryService::Shutdown: Crash recovery service shutdown");
}

bool CrashRecoveryService::ExecuteWithTimeout(std::function<void()> func, int timeoutMs, const std::string& operationName) {
    logger_->Trace("CrashRecoveryService", "ExecuteWithTimeout", "timeoutMs=" + std::to_string(timeoutMs) + ", operationName=" + operationName, "Executing with timeout");

    std::promise<void> completionPromise;
    auto completionFuture = completionPromise.get_future();
    std::thread workerThread([task = std::move(func), promise = std::move(completionPromise)]() mutable {
        try {
            task();
            promise.set_value();
        } catch (...) {
            try {
                promise.set_exception(std::current_exception());
            } catch (...) {
                // Swallow exception if promise is already satisfied.
            }
        }
    });

    const auto start = std::chrono::steady_clock::now();
    const auto timeoutDuration = std::chrono::milliseconds(timeoutMs);
    const auto pollInterval = std::chrono::milliseconds(50);

    while (true) {
        if (completionFuture.wait_for(pollInterval) == std::future_status::ready) {
            try {
                completionFuture.get(); // Re-throw any exceptions
                logger_->Trace("CrashRecoveryService", "ExecuteWithTimeout", "", "Operation completed successfully");
                if (workerThread.joinable()) {
                    workerThread.join();
                }
                return true;
            } catch (const std::exception& e) {
                logger_->Error("CrashRecoveryService::ExecuteWithTimeout: Operation '" + operationName + "' threw exception: " + e.what());
                if (workerThread.joinable()) {
                    workerThread.join();
                }
                throw;
            } catch (...) {
                logger_->Error("CrashRecoveryService::ExecuteWithTimeout: Operation '" + operationName + "' threw unknown exception");
                if (workerThread.joinable()) {
                    workerThread.join();
                }
                throw;
            }
        }

        if (crashDetected_.load()) {
            logger_->Warn("CrashRecoveryService::ExecuteWithTimeout: Operation '" + operationName + "' aborted after crash detection");
            if (workerThread.joinable()) {
                workerThread.detach();
            }
            return false;
        }

        if (timeoutMs >= 0 && std::chrono::steady_clock::now() - start >= timeoutDuration) {
            logger_->Warn("CrashRecoveryService::ExecuteWithTimeout: Operation '" + operationName + "' timed out after " + std::to_string(timeoutMs) + "ms");
            logger_->Trace("CrashRecoveryService", "ExecuteWithTimeout",
                           "timeoutMs=" + std::to_string(timeoutMs) + ", operationName=" + operationName,
                           "Timeout detected; marking crash and detaching worker thread");

            {
                std::lock_guard<std::mutex> lock(crashMutex_);
                crashDetected_ = true;
                crashReport_ += "\nTIMEOUT: Operation '" + operationName + "' exceeded " +
                               std::to_string(timeoutMs) + "ms\n";
            }

            // No safe way to cancel; detach so we can report the timeout promptly.
            if (workerThread.joinable()) {
                workerThread.detach();
            }

            return false;
        }
    }
}

void CrashRecoveryService::RecordFrameHeartbeat(double frameTimeSeconds) {
    lastHeartbeatNs_.store(GetSteadyClockNs(), std::memory_order_relaxed);
    heartbeatSeen_.store(true, std::memory_order_release);
    lastSuccessfulFrameTime_ = frameTimeSeconds;
}

void CrashRecoveryService::MonitorHeartbeats() {
    logger_->Trace("CrashRecoveryService", "MonitorHeartbeats", "", "Starting heartbeat monitor");

    const int64_t timeoutNs = heartbeatTimeout_.count() * 1000 * 1000;
    while (heartbeatMonitorRunning_.load()) {
        std::this_thread::sleep_for(heartbeatPollInterval_);
        if (!heartbeatSeen_.load(std::memory_order_acquire)) {
            continue;
        }

        const int64_t lastHeartbeat = lastHeartbeatNs_.load(std::memory_order_relaxed);
        if (lastHeartbeat == 0) {
            continue;
        }

        const int64_t nowNs = GetSteadyClockNs();
        const int64_t elapsedNs = nowNs - lastHeartbeat;
        if (elapsedNs >= timeoutNs) {
            const int64_t elapsedMs = elapsedNs / (1000 * 1000);
            std::lock_guard<std::mutex> lock(crashMutex_);
            if (!crashDetected_) {
                crashDetected_ = true;
                crashReport_ += "\nHEARTBEAT TIMEOUT: No frame heartbeat for " +
                               std::to_string(elapsedMs) + "ms\n";
                logger_->Error("CrashRecoveryService::MonitorHeartbeats: Frame heartbeat stalled for " +
                              std::to_string(elapsedMs) + "ms");
            }
        }
    }

    logger_->Trace("CrashRecoveryService", "MonitorHeartbeats", "", "Heartbeat monitor exiting");
}

bool CrashRecoveryService::IsCrashDetected() const {
    logger_->Trace("CrashRecoveryService", "IsCrashDetected");
    return crashDetected_.load();
}

bool CrashRecoveryService::AttemptRecovery() {
    logger_->Trace("CrashRecoveryService", "AttemptRecovery", "", "Attempting recovery");

    std::lock_guard<std::mutex> lock(crashMutex_);

    if (!crashDetected_) {
        logger_->Warn("CrashRecoveryService::AttemptRecovery: No crash detected");
        return true;
    }

    bool recovered = PerformRecovery();
    if (recovered) {
        crashDetected_ = false;
        lastSignal_ = 0;
        crashReport_.clear();
        lastHeartbeatNs_ = 0;
        heartbeatSeen_ = false;
        logger_->Info("CrashRecoveryService::AttemptRecovery: Recovery successful");
    } else {
        logger_->Error("CrashRecoveryService::AttemptRecovery: Recovery failed");
    }

    return recovered;
}

std::string CrashRecoveryService::GetCrashReport() const {
    logger_->Trace("CrashRecoveryService", "GetCrashReport");
    std::lock_guard<std::mutex> lock(crashMutex_);
    return crashReport_;
}

void CrashRecoveryService::SignalHandler(int signal) {
    if (crashHandlingInProgress != 0) {
        _exit(kSignalExitCodeBase + signal);
    }
    crashHandlingInProgress = 1;

    if (instance_) {
        if (instance_->logger_) {
            instance_->logger_->Trace("CrashRecoveryService", "SignalHandler",
                                      "signal=" + std::to_string(signal));
        }
        instance_->HandleCrash(signal);
    }

    _exit(kSignalExitCodeBase + signal);
}

void CrashRecoveryService::SetupSignalHandlers() {
    logger_->Trace("CrashRecoveryService", "SetupSignalHandlers");
    if (signalHandlersInstalled_) {
        return;
    }

    instance_ = this;

    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SignalHandler;
    sa.sa_flags = SA_RESTART;

    // Install handlers for common crash signals
    if (sigaction(SIGSEGV, &sa, &oldSigsegv_) == -1) {
        logger_->Warn("CrashRecoveryService::SetupSignalHandlers: Failed to install SIGSEGV handler");
    }
    if (sigaction(SIGABRT, &sa, &oldSigabrt_) == -1) {
        logger_->Warn("CrashRecoveryService::SetupSignalHandlers: Failed to install SIGABRT handler");
    }
    if (sigaction(SIGFPE, &sa, &oldSigfpe_) == -1) {
        logger_->Warn("CrashRecoveryService::SetupSignalHandlers: Failed to install SIGFPE handler");
    }
    if (sigaction(SIGILL, &sa, &oldSigill_) == -1) {
        logger_->Warn("CrashRecoveryService::SetupSignalHandlers: Failed to install SIGILL handler");
    }

    signalHandlersInstalled_ = true;
    logger_->Info("CrashRecoveryService::SetupSignalHandlers: Signal handlers installed");
}

void CrashRecoveryService::RemoveSignalHandlers() {
    logger_->Trace("CrashRecoveryService", "RemoveSignalHandlers");
    if (!signalHandlersInstalled_) {
        return;
    }

    // Restore original signal handlers
    sigaction(SIGSEGV, &oldSigsegv_, nullptr);
    sigaction(SIGABRT, &oldSigabrt_, nullptr);
    sigaction(SIGFPE, &oldSigfpe_, nullptr);
    sigaction(SIGILL, &oldSigill_, nullptr);

    signalHandlersInstalled_ = false;
    instance_ = nullptr;

    logger_->Info("CrashRecoveryService::RemoveSignalHandlers: Signal handlers removed");
}

void CrashRecoveryService::HandleCrash(int signal) {
    logger_->Trace("CrashRecoveryService", "HandleCrash",
                   "signal=" + std::to_string(signal));
    std::lock_guard<std::mutex> lock(crashMutex_);

    crashDetected_ = true;
    lastSignal_ = signal;

    std::stringstream ss;
    ss << "Crash detected! Signal: " << signal << " (";

    switch (signal) {
        case SIGSEGV: ss << "SIGSEGV - Segmentation fault"; break;
        case SIGABRT: ss << "SIGABRT - Abort signal"; break;
        case SIGFPE: ss << "SIGFPE - Floating point exception"; break;
        case SIGILL: ss << "SIGILL - Illegal instruction"; break;
        default: ss << "Unknown signal"; break;
    }

    ss << ")\nProcess ID: " << getpid();
    ss << "\nThread ID: " << std::this_thread::get_id();

    crashReport_ = ss.str();

    const std::string stackTrace = BuildStackTrace();
    if (!stackTrace.empty()) {
        crashReport_ += "\n=== STACK TRACE ===\n";
        crashReport_ += stackTrace;
    }

    logger_->Error("CrashRecoveryService::HandleCrash: " + crashReport_);

    // Note: In a real implementation, you might want to:
    // 1. Generate a core dump
    // 2. Send crash report to monitoring service
    // 3. Attempt graceful shutdown
    // 4. Restart critical services

    // For now, we just log and set the flag
}

bool CrashRecoveryService::PerformRecovery() {
    logger_->Trace("CrashRecoveryService", "PerformRecovery");
    // Basic recovery logic - in a real implementation this would be more sophisticated
    logger_->Info("CrashRecoveryService::PerformRecovery: Performing basic recovery");

    // Reset crash state
    // In a more advanced implementation, this might:
    // - Restart failed services
    // - Reset corrupted state
    // - Reinitialize resources
    // - Restore from backup

    logger_->Info("CrashRecoveryService::PerformRecovery: Recovery completed");
    return true;
}

bool CrashRecoveryService::CheckGpuHealth(double lastFrameTime, double expectedFrameTime) {
    logger_->Trace("CrashRecoveryService", "CheckGpuHealth",
                   "lastFrameTime=" + std::to_string(lastFrameTime) +
                   ", expectedFrameTime=" + std::to_string(expectedFrameTime));
    UpdateHealthMetrics();

    if (config_.gpuHangFrameTimeMultiplier <= 0.0 || config_.maxConsecutiveGpuTimeouts == 0) {
        consecutiveFrameTimeouts_ = 0;
        lastSuccessfulFrameTime_ = lastFrameTime;
        return true;
    }

    // Check for GPU hangs - if we haven't had a successful frame in too long
    if (lastFrameTime > expectedFrameTime * config_.gpuHangFrameTimeMultiplier) {
        consecutiveFrameTimeouts_++;
        if (consecutiveFrameTimeouts_ > config_.maxConsecutiveGpuTimeouts) {
            std::lock_guard<std::mutex> lock(crashMutex_);
            crashDetected_ = true;
            crashReport_ += "\nGPU HANG DETECTED: No successful frames for " +
                           std::to_string(lastFrameTime) + " seconds\n";
            crashReport_ += "Expected frame time: " + std::to_string(expectedFrameTime) + " seconds\n";
            crashReport_ += "Consecutive timeouts: " + std::to_string(consecutiveFrameTimeouts_) + "\n";

            logger_->Error("CrashRecoveryService::CheckGpuHealth: GPU hang detected - " +
                          std::to_string(lastFrameTime) + " seconds since last frame");
            return false;
        }
    } else {
        consecutiveFrameTimeouts_ = 0;
        lastSuccessfulFrameTime_ = lastFrameTime;
    }

    return true;
}

bool CrashRecoveryService::ValidateFileFormat(const std::string& filePath, const std::string& expectedFormat) {
    logger_->Trace("CrashRecoveryService", "ValidateFileFormat",
                   "filePath=" + filePath +
                   ", expectedFormat=" + expectedFormat);
    // Basic file format validation
    std::filesystem::path path(filePath);

    if (!std::filesystem::exists(path)) {
        fileFormatErrors_++;
        std::lock_guard<std::mutex> lock(crashMutex_);
        crashReport_ += "\nFILE NOT FOUND: " + filePath + "\n";
        logger_->Error("CrashRecoveryService::ValidateFileFormat: File not found: " + filePath);
        return false;
    }

    std::string extension = path.extension().string();
    if (!expectedFormat.empty() && extension != expectedFormat) {
        fileFormatErrors_++;
        std::lock_guard<std::mutex> lock(crashMutex_);
        crashReport_ += "\nINVALID FILE FORMAT: " + filePath + "\n";
        crashReport_ += "Expected: " + expectedFormat + ", Got: " + extension + "\n";

        logger_->Warn("CrashRecoveryService::ValidateFileFormat: Invalid format for " +
                     filePath + " - expected " + expectedFormat + ", got " + extension);

        if (config_.maxFileFormatErrors > 0 && fileFormatErrors_ > config_.maxFileFormatErrors) {
            crashDetected_ = true;
            crashReport_ += "CRITICAL: Multiple file format errors detected\n";
        }

        return false;
    }

    // Check file size (basic corruption detection)
    try {
        auto fileSize = std::filesystem::file_size(path);
        if (fileSize == 0) {
            fileFormatErrors_++;
            std::lock_guard<std::mutex> lock(crashMutex_);
            crashReport_ += "\nEMPTY FILE: " + filePath + "\n";
            logger_->Error("CrashRecoveryService::ValidateFileFormat: Empty file: " + filePath);
            return false;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        fileFormatErrors_++;
        std::lock_guard<std::mutex> lock(crashMutex_);
        crashReport_ += "\nFILE ACCESS ERROR: " + filePath + " - " + e.what() + "\n";
        logger_->Error("CrashRecoveryService::ValidateFileFormat: File access error for " +
                      filePath + ": " + e.what());
        return false;
    }

    return true;
}

bool CrashRecoveryService::CheckMemoryHealth() {
    logger_->Trace("CrashRecoveryService", "CheckMemoryHealth");
    UpdateHealthMetrics();

    if (memoryLimitBytes_ == 0) {
        return true;
    }

    size_t currentMemory = GetCurrentMemoryUsage();

    // Basic memory usage check (this is a simplified version)
    // In a real implementation, you'd track memory growth over time
    if (currentMemory > memoryLimitBytes_) {
        memoryWarnings_++;
        std::lock_guard<std::mutex> lock(crashMutex_);
        crashReport_ += "\nHIGH MEMORY USAGE: " + std::to_string(currentMemory / (1024*1024)) + " MB\n";

        logger_->Warn("CrashRecoveryService::CheckMemoryHealth: High memory usage detected: " +
                     std::to_string(currentMemory / (1024*1024)) + " MB");

        if (config_.maxMemoryWarnings > 0 && memoryWarnings_ > config_.maxMemoryWarnings) {
            crashDetected_ = true;
            crashReport_ += "CRITICAL: Persistent high memory usage detected\n";
            return false;
        }
    }

    return true;
}

std::string CrashRecoveryService::GetSystemHealthStatus() const {
    logger_->Trace("CrashRecoveryService", "GetSystemHealthStatus");
    std::stringstream ss;

    ss << "=== SYSTEM HEALTH STATUS ===\n";
    ss << "Crash Detected: " << (crashDetected_ ? "YES" : "NO") << "\n";
    ss << "Last Signal: " << lastSignal_ << "\n";
    ss << "Consecutive Frame Timeouts: " << consecutiveFrameTimeouts_ << "\n";
    ss << "File Format Errors: " << fileFormatErrors_ << "\n";
    ss << "Memory Warnings: " << memoryWarnings_ << "\n";
    ss << "Last Successful Frame Time: " << lastSuccessfulFrameTime_ << " seconds\n";
    if (heartbeatSeen_.load(std::memory_order_acquire)) {
        const int64_t lastHeartbeat = lastHeartbeatNs_.load(std::memory_order_relaxed);
        const int64_t ageMs = (GetSteadyClockNs() - lastHeartbeat) / (1000 * 1000);
        ss << "Last Frame Heartbeat Age: " << ageMs << " ms\n";
    } else {
        ss << "Last Frame Heartbeat Age: n/a\n";
    }

    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastCheck = std::chrono::duration_cast<std::chrono::seconds>(
        now - lastHealthCheck_).count();
    ss << "Time Since Last Health Check: " << timeSinceLastCheck << " seconds\n";

    if (!crashReport_.empty()) {
        ss << "\n=== CRASH REPORT ===\n" << crashReport_;
    }

    return ss.str();
}

void CrashRecoveryService::UpdateHealthMetrics() {
    logger_->Trace("CrashRecoveryService", "UpdateHealthMetrics");
    lastHealthCheck_ = std::chrono::steady_clock::now();
}

size_t CrashRecoveryService::GetCurrentMemoryUsage() const {
    logger_->Trace("CrashRecoveryService", "GetCurrentMemoryUsage");
    // This is a simplified memory usage check
    // In a real implementation, you'd use platform-specific APIs
    // For Linux, you might read /proc/self/status or use getrusage

    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        // Return resident set size in bytes
        if (usage.ru_maxrss <= 0) {
            return 0;
        }
        return static_cast<size_t>(usage.ru_maxrss) * 1024; // ru_maxrss is in KB on Linux
    }

    return 0;
}

bool CrashRecoveryService::IsGpuResponsive() const {
    logger_->Trace("CrashRecoveryService", "IsGpuResponsive");
    // This is a placeholder for GPU responsiveness checking
    // In a real implementation, you'd try a simple GPU operation
    // and check if it completes within a reasonable time

    return consecutiveFrameTimeouts_ < 3;
}

} // namespace sdl3cpp::services::impl
