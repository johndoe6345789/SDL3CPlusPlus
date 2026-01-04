#include "crash_recovery_service.hpp"
#include <future>
#include <chrono>
#include <sstream>
#include <cstring>
#include <unistd.h>

namespace sdl3cpp::services::impl {

// Static instance for signal handler
CrashRecoveryService* CrashRecoveryService::instance_ = nullptr;

CrashRecoveryService::CrashRecoveryService(std::shared_ptr<ILogger> logger)
    : logger_(logger)
    , crashDetected_(false)
    , lastSignal_(0)
    , signalHandlersInstalled_(false) {
    logger_->Trace("CrashRecoveryService", "CrashRecoveryService", "", "Created");
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
    crashReport_.clear();

    logger_->Info("CrashRecoveryService::Initialize: Crash recovery service initialized");
}

void CrashRecoveryService::Shutdown() {
    logger_->Trace("CrashRecoveryService", "Shutdown", "", "Shutting down crash recovery service");

    RemoveSignalHandlers();

    logger_->Info("CrashRecoveryService::Shutdown: Crash recovery service shutdown");
}

bool CrashRecoveryService::ExecuteWithTimeout(std::function<void()> func, int timeoutMs, const std::string& operationName) {
    logger_->Trace("CrashRecoveryService", "ExecuteWithTimeout", "timeoutMs=" + std::to_string(timeoutMs) + ", operationName=" + operationName, "Executing with timeout");

    auto future = std::async(std::launch::async, func);

    if (future.wait_for(std::chrono::milliseconds(timeoutMs)) == std::future_status::timeout) {
        logger_->Warn("CrashRecoveryService::ExecuteWithTimeout: Operation '" + operationName + "' timed out after " + std::to_string(timeoutMs) + "ms");

        // Attempt to cancel the operation (limited effectiveness)
        // Note: std::future doesn't provide direct cancellation, this is just detection

        return false;
    }

    try {
        future.get(); // Re-throw any exceptions
        logger_->Trace("CrashRecoveryService", "ExecuteWithTimeout", "", "Operation completed successfully");
        return true;
    } catch (const std::exception& e) {
        logger_->Error("CrashRecoveryService::ExecuteWithTimeout: Operation '" + operationName + "' threw exception: " + e.what());
        throw;
    }
}

bool CrashRecoveryService::IsCrashDetected() const {
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
        logger_->Info("CrashRecoveryService::AttemptRecovery: Recovery successful");
    } else {
        logger_->Error("CrashRecoveryService::AttemptRecovery: Recovery failed");
    }

    return recovered;
}

std::string CrashRecoveryService::GetCrashReport() const {
    std::lock_guard<std::mutex> lock(crashMutex_);
    return crashReport_;
}

void CrashRecoveryService::SignalHandler(int signal) {
    if (instance_) {
        instance_->HandleCrash(signal);
    }
}

void CrashRecoveryService::SetupSignalHandlers() {
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

    logger_->Error("CrashRecoveryService::HandleCrash: " + crashReport_);

    // Note: In a real implementation, you might want to:
    // 1. Generate a core dump
    // 2. Send crash report to monitoring service
    // 3. Attempt graceful shutdown
    // 4. Restart critical services

    // For now, we just log and set the flag
}

bool CrashRecoveryService::PerformRecovery() {
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

} // namespace sdl3cpp::services::impl