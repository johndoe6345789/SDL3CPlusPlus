#pragma once

#include <functional>
#include <string>

namespace sdl3cpp::services {

/**
 * @brief Crash recovery service interface.
 *
 * Provides mechanisms for detecting and recovering from crashes and infinite loops.
 * Small, focused service (~30 lines) for application stability.
 */
class ICrashRecoveryService {
public:
    virtual ~ICrashRecoveryService() = default;

    /**
     * @brief Initialize crash recovery mechanisms.
     */
    virtual void Initialize() = 0;

    /**
     * @brief Shutdown crash recovery mechanisms.
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Execute a function with timeout protection.
     *
     * @param func Function to execute
     * @param timeoutMs Timeout in milliseconds
     * @param operationName Name of the operation for logging
     * @return true if function completed successfully, false if timeout occurred
     */
    virtual bool ExecuteWithTimeout(std::function<void()> func, int timeoutMs, const std::string& operationName) = 0;

    /**
     * @brief Check if a crash has been detected.
     *
     * @return true if crash detected
     */
    virtual bool IsCrashDetected() const = 0;

    /**
     * @brief Attempt recovery from detected crash.
     *
     * @return true if recovery successful
     */
    virtual bool AttemptRecovery() = 0;

    /**
     * @brief Get crash report.
     *
     * @return Crash report string
     */
    virtual std::string GetCrashReport() const = 0;
};

} // namespace sdl3cpp::services