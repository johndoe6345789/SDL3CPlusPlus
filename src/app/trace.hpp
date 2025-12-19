#ifndef SDL3CPP_APP_TRACE_HPP
#define SDL3CPP_APP_TRACE_HPP

#include <atomic>
#include <iostream>

namespace sdl3cpp::app {

class TraceLogger {
public:
    static void SetEnabled(bool enabled) noexcept {
        enabled_.store(enabled, std::memory_order_relaxed);
    }

    static bool Enabled() noexcept {
        return enabled_.load(std::memory_order_relaxed);
    }

    static void Log(const char* message) {
        if (Enabled()) {
            std::cout << "[TRACE] " << message << '\n';
        }
    }

private:
    static inline std::atomic_bool enabled_{false};
};

class TraceScope {
public:
    explicit TraceScope(const char* name) : name_(name) {
        TraceLogger::Log(name_);
    }

private:
    const char* name_;
};

} // namespace sdl3cpp::app

#define TRACE_FUNCTION() sdl3cpp::app::TraceScope traceScope##__COUNTER__{__func__}

#endif // SDL3CPP_APP_TRACE_HPP
