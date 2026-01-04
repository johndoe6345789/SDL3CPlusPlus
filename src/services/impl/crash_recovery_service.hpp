#pragma once

#include "../interfaces/i_crash_recovery_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <csignal>
#include <string>
#include <functional>

namespace sdl3cpp::services::impl {

/**
 * @brief Crash recovery service implementation.
 *
 * Detects crashes and infinite loops, provides recovery mechanisms.
 * Uses signal handlers and timeout monitoring.
 */
class CrashRecoveryService : public ICrashRecoveryService {
public:
    explicit CrashRecoveryService(std::shared_ptr<ILogger> logger);
    ~CrashRecoveryService() override;

    // ICrashRecoveryService interface
    void Initialize() override;
    void Shutdown() override;
    bool ExecuteWithTimeout(std::function<void()> func, int timeoutMs, const std::string& operationName) override;
    bool IsCrashDetected() const override;
    bool AttemptRecovery() override;
    std::string GetCrashReport() const override;

private:
    // Signal handling
    static void SignalHandler(int signal);
    void SetupSignalHandlers();
    void RemoveSignalHandlers();

    // Crash detection and recovery
    void HandleCrash(int signal);
    bool PerformRecovery();

    std::shared_ptr<ILogger> logger_;
    std::atomic<bool> crashDetected_;
    std::atomic<int> lastSignal_;
    std::string crashReport_;
    mutable std::mutex crashMutex_;

    // Signal handler state
    static CrashRecoveryService* instance_;
    struct sigaction oldSigsegv_;
    struct sigaction oldSigabrt_;
    struct sigaction oldSigfpe_;
    struct sigaction oldSigill_;
    bool signalHandlersInstalled_;
};

} // namespace sdl3cpp::services::impl