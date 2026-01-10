#include <atomic>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include <SDL3/SDL_main.h>

#include "app/service_based_app.hpp"
#include "services/impl/app/command_line_service.hpp"
#include "services/impl/config/json_config_writer_service.hpp"
#include "services/impl/diagnostics/logger_service.hpp"
#include "services/impl/platform/platform_service.hpp"
#include "services/interfaces/i_logger.hpp"

namespace sdl3cpp::app {
std::atomic<bool> g_signalReceived{false};
}

namespace {

void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        sdl3cpp::app::g_signalReceived.store(true);
    }
}

void SetupSignalHandlers() {
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
}

} // namespace

int main(int argc, char** argv) {
    SDL_SetMainReady();
    SetupSignalHandlers();
    try {
        auto startupLogger = std::make_shared<sdl3cpp::services::impl::LoggerService>();
        auto platformService = std::make_shared<sdl3cpp::services::impl::PlatformService>(startupLogger);
        sdl3cpp::services::impl::CommandLineService commandLineService(startupLogger, platformService);
        sdl3cpp::services::CommandLineOptions options = commandLineService.Parse(argc, argv);

        sdl3cpp::services::LogLevel logLevel = options.traceEnabled
            ? sdl3cpp::services::LogLevel::TRACE
            : sdl3cpp::services::LogLevel::INFO;
        sdl3cpp::app::ServiceBasedApp app(options.runtimeConfig, logLevel);
        app.ConfigureLogging(logLevel, true, "sdl3_app.log");

        auto logger = app.GetLogger();
        if (logger) {
            logger->Info("Application starting");
            logger->TraceVariable("config.width", static_cast<int>(options.runtimeConfig.width));
            logger->TraceVariable("config.height", static_cast<int>(options.runtimeConfig.height));
            logger->TraceVariable("config.projectRoot", options.runtimeConfig.projectRoot.string());
            logger->TraceVariable("config.windowTitle", options.runtimeConfig.windowTitle);
        }

        if (options.seedOutput || options.saveDefaultJson) {
            sdl3cpp::services::impl::JsonConfigWriterService configWriter(logger);
            if (options.seedOutput) {
                configWriter.WriteConfig(options.runtimeConfig, *options.seedOutput);
            }
            if (options.saveDefaultJson) {
                if (auto configDir = platformService->GetUserConfigDirectory()) {
                    configWriter.WriteConfig(options.runtimeConfig, *configDir / "default_runtime.json");
                } else {
                    throw std::runtime_error("Unable to determine platform config directory");
                }
            }
        }

        app.Run();
    } catch (const std::runtime_error& e) {
        std::string errorMsg = e.what();
        // For early errors before app is created, we can't use service logger
        // Fall back to console output
        std::cerr << "Runtime error: " << errorMsg << std::endl;

        // Check if this is a timeout/hang error - show simpler message for these
        bool isTimeoutError = errorMsg.find("timeout") != std::string::npos ||
                              errorMsg.find("Launch timeout") != std::string::npos ||
                              errorMsg.find("Swapchain recreation loop") != std::string::npos;

        if (!isTimeoutError) {
            // For non-timeout errors, show full error dialog
            SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR,
                "Application Error",
                errorMsg.c_str(),
                nullptr);
        } else {
            // For timeout errors, the console output already has diagnostic info
            // Just show a brief dialog
            std::string briefMsg = "Application failed to launch. Check console output for details.";
            SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR,
                "Launch Failed",
                briefMsg.c_str(),
                nullptr);
        }
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        // For early errors before app is created, we can't use service logger
        std::cerr << "Exception: " << e.what() << std::endl;
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Application Error",
            e.what(),
            nullptr);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
