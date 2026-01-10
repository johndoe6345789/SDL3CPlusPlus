#pragma once

#include "services/interfaces/config_types.hpp"
#include "services/interfaces/i_crash_recovery_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <csignal>
#include <chrono>
#include <string>
#include <functional>

namespace sdl3cpp::services::impl {

/**
 * @brief Crash recovery service implementation.
 *
 * Detects crashes, GPU hangs, file format issues, and provides recovery mechanisms.
 * Monitors system health and provides comprehensive error detection and recovery.
 */
class CrashRecoveryService : public ICrashRecoveryService {
public:
    CrashRecoveryService(std::shared_ptr<ILogger> logger, CrashRecoveryConfig config);
    ~CrashRecoveryService() override;

    // ICrashRecoveryService interface
    void Initialize() override;
    void Shutdown() override;
    bool ExecuteWithTimeout(std::function<void()> func, int timeoutMs, const std::string& operationName) override;
    void RecordFrameHeartbeat(double frameTimeSeconds) override;
    bool IsCrashDetected() const override;
    bool AttemptRecovery() override;
    std::string GetCrashReport() const override;
    bool CheckGpuHealth(double lastFrameTime, double expectedFrameTime) override;
    bool ValidateFileFormat(const std::string& filePath, const std::string& expectedFormat) override;
    bool CheckMemoryHealth() override;
    std::string GetSystemHealthStatus() const override;

private:
    // Signal handling
    static void SignalHandler(int signal);
    void SetupSignalHandlers();
    void RemoveSignalHandlers();

    // Crash detection and recovery
    void HandleCrash(int signal);
    bool PerformRecovery();

    // Health monitoring
    void UpdateHealthMetrics();
    size_t GetCurrentMemoryUsage() const;
    bool IsGpuResponsive() const;
    void MonitorHeartbeats();

    std::shared_ptr<ILogger> logger_;
    std::atomic<bool> crashDetected_;
    std::atomic<int> lastSignal_;
    std::atomic<int64_t> lastHeartbeatNs_;
    std::atomic<bool> heartbeatSeen_;
    std::atomic<bool> heartbeatMonitorRunning_;
    std::thread heartbeatMonitorThread_;
    std::chrono::milliseconds heartbeatTimeout_{5000};
    std::chrono::milliseconds heartbeatPollInterval_{200};
    std::string crashReport_;
    mutable std::mutex crashMutex_;
    CrashRecoveryConfig config_{};
    size_t memoryLimitBytes_ = 0;

    // Health monitoring state
    std::atomic<double> lastSuccessfulFrameTime_;
    std::atomic<size_t> consecutiveFrameTimeouts_;
    std::atomic<size_t> fileFormatErrors_;
    std::atomic<size_t> memoryWarnings_;
    std::chrono::steady_clock::time_point lastHealthCheck_;

    // Signal handler state
    static CrashRecoveryService* instance_;
    struct sigaction oldSigsegv_;
    struct sigaction oldSigabrt_;
    struct sigaction oldSigfpe_;
    struct sigaction oldSigill_;
    bool signalHandlersInstalled_;
};

} // namespace sdl3cpp::services::impl
