#include <gtest/gtest.h>

#include "services/impl/diagnostics/crash_recovery_service.hpp"

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>

namespace {

class NullLogger final : public sdl3cpp::services::ILogger {
public:
    void SetLevel(sdl3cpp::services::LogLevel) override {}
    sdl3cpp::services::LogLevel GetLevel() const override { return sdl3cpp::services::LogLevel::OFF; }
    void SetOutputFile(const std::string&) override {}
    void SetMaxLinesPerFile(size_t) override {}
    void EnableConsoleOutput(bool) override {}
    void Log(sdl3cpp::services::LogLevel, const std::string&) override {}
    void Trace(const std::string&) override {}
    void Trace(const std::string&, const std::string&, const std::string&, const std::string&) override {}
    void Debug(const std::string&) override {}
    void Info(const std::string&) override {}
    void Warn(const std::string&) override {}
    void Error(const std::string&) override {}
    void TraceFunction(const std::string&) override {}
    void TraceVariable(const std::string&, const std::string&) override {}
    void TraceVariable(const std::string&, int) override {}
    void TraceVariable(const std::string&, size_t) override {}
    void TraceVariable(const std::string&, bool) override {}
    void TraceVariable(const std::string&, float) override {}
    void TraceVariable(const std::string&, double) override {}
};

// TDD FAILING TEST:
// sdl3_app.log ends with "Main Application Loop" timing out after 30000ms.
// ExecuteWithTimeout should return promptly after the timeout instead of
// blocking on the background task's completion.
TEST(CrashRecoveryTimeoutTest, ExecuteWithTimeoutReturnsPromptlyAfterTimeout) {
    auto logger = std::make_shared<NullLogger>();
    sdl3cpp::services::CrashRecoveryConfig config;
    sdl3cpp::services::impl::CrashRecoveryService crashRecoveryService(logger, config);

    auto completionPromise = std::make_shared<std::promise<void>>();
    auto completionFuture = completionPromise->get_future();

    const auto start = std::chrono::steady_clock::now();
    const bool success = crashRecoveryService.ExecuteWithTimeout(
        [promise = completionPromise]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            promise->set_value();
        },
        10,
        "Main Application Loop");
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - start)
                               .count();

    EXPECT_FALSE(success);
    EXPECT_LT(elapsedMs, 100)
        << "ExecuteWithTimeout should return shortly after the timeout instead of waiting for completion.";

    EXPECT_EQ(completionFuture.wait_for(std::chrono::milliseconds(500)), std::future_status::ready)
        << "Background task should finish promptly to avoid leaking threads during tests.";
}

} // namespace
