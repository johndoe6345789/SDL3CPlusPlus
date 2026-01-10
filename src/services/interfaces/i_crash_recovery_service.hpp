#pragma once

#include <functional>
#include <string>

namespace sdl3cpp::services {

/**
 * @brief Crash recovery service interface.
 *
 * Provides mechanisms for detecting and recovering from crashes and infinite loops.
 * Monitors GPU state, file operations, and other critical subsystems.
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
     * @brief Record a heartbeat for the main loop or long-running operation.
     *
     * @param frameTimeSeconds Delta time for the last frame in seconds
     */
    virtual void RecordFrameHeartbeat(double frameTimeSeconds) = 0;

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

    /**
     * @brief Monitor GPU health and detect hangs.
     *
     * @param lastFrameTime Time since last successful frame
     * @param expectedFrameTime Expected frame time in seconds
     * @return true if GPU appears hung
     */
    virtual bool CheckGpuHealth(double lastFrameTime, double expectedFrameTime = 1.0/60.0) = 0;

    /**
     * @brief Check file format validity.
     *
     * @param filePath Path to the file
     * @param expectedFormat Expected file format/extension
     * @return true if file format is valid
     */
    virtual bool ValidateFileFormat(const std::string& filePath, const std::string& expectedFormat) = 0;

    /**
     * @brief Monitor memory usage and detect leaks.
     *
     * @return true if memory usage is within safe limits
     */
    virtual bool CheckMemoryHealth() = 0;

    /**
     * @brief Get system health status.
     *
     * @return Health status string
     */
    virtual std::string GetSystemHealthStatus() const = 0;
};

} // namespace sdl3cpp::services
