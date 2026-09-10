#include "services/interfaces/app/app_bootstrap.hpp"

#include "services/interfaces/diagnostics/logger_service.hpp"

namespace sdl3cpp::services::app {

std::shared_ptr<ILogger> CreateAppLogger(
    const std::filesystem::path& projectRoot, bool traceEnabled) {
    auto logger = std::make_shared<impl::LoggerService>();
    logger->EnableConsoleOutput(false);
    std::filesystem::path logPath = projectRoot / "sdl3_app.log";
    logger->SetOutputFile(logPath.string());
    if (traceEnabled) {
        logger->SetLevel(LogLevel::TRACE);
    }
    return logger;
}

}  // namespace sdl3cpp::services::app
