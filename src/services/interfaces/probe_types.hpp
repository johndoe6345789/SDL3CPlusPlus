#pragma once

#include <string>

namespace sdl3cpp::services {

/**
 * @brief Severity levels for probe reports.
 */
enum class ProbeSeverity {
    Info,
    Warn,
    Error,
    Fatal
};

/**
 * @brief Structured probe report for diagnostics and policy handling.
 */
struct ProbeReport {
    ProbeSeverity severity = ProbeSeverity::Info;
    std::string code;
    std::string jsonPath;
    std::string renderPass;
    std::string resourceId;
    std::string message;
    std::string details;
};

}  // namespace sdl3cpp::services
