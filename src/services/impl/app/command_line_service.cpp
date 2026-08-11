#include "services/interfaces/app/command_line_service.hpp"
#include <CLI/CLI.hpp>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>
#include <filesystem>

namespace sdl3cpp::services::impl {

CommandLineService::CommandLineService(std::shared_ptr<ILogger> logger,
                                       std::shared_ptr<IPlatformService> platformService)
    : logger_(std::move(logger)),
      platformService_(std::move(platformService)) {
    if (!logger_) {
        throw std::runtime_error("CommandLineService requires a logger");
    }
    if (logger_) {
        logger_->Trace("CommandLineService", "CommandLineService",
                       "platformService=" + std::string(platformService_ ? "set" : "null"),
                       "Created");
    }
}

CommandLineOptions CommandLineService::Parse(int argc, char** argv) {
    CommandLineOptions result;
    result.runtimeConfig.width = 1024;
    result.runtimeConfig.height = 768;
    result.runtimeConfig.windowTitle = "SDL3 Game Engine";
    result.runtimeConfig.projectRoot = std::filesystem::current_path();
    result.traceEnabled = false;
    result.bootstrapPackage = "bootstrap";
    result.gamePackage = "standalone_cubes";

    CLI::App app{"SDL3 Game Engine"};
    app.add_option("--bootstrap", result.bootstrapPackage, "Bootstrap package name");
    app.add_option("--game", result.gamePackage, "Game package to load");
    app.add_option("-w,--width", result.runtimeConfig.width, "Window width");
    app.add_option("--height", result.runtimeConfig.height, "Window height");  // No -h short flag (conflicts with help)
    app.add_option("--title", result.runtimeConfig.windowTitle, "Window title");
    app.add_flag("--trace", result.traceEnabled, "Enable trace logging");
    app.add_option("--project-root", result.runtimeConfig.projectRoot, "Project root directory");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        if (logger_) {
            logger_->Error("CommandLineService::Parse: " + std::string(e.what()));
        }
        throw std::runtime_error("Failed to parse command line arguments");
    }

    return result;
}

std::optional<std::filesystem::path> CommandLineService::GetDefaultConfigPath() const {
    return std::nullopt;
}

RuntimeConfig CommandLineService::LoadConfigFromJson(const std::filesystem::path& configPath,
                                                      bool dumpConfig) {
    if (logger_) {
        logger_->Trace("CommandLineService", "LoadConfigFromJson",
                       "configPath=" + configPath.string() +
                       ", dumpConfig=" + std::string(dumpConfig ? "true" : "false"));
    }
    RuntimeConfig config;
    config.projectRoot = std::filesystem::current_path();
    config.width = 1024;
    config.height = 768;
    config.windowTitle = "SDL3 Game Engine";
    return config;
}

RuntimeConfig CommandLineService::LoadDefaultConfig(const char* argv0) {
    if (logger_) {
        logger_->Trace("CommandLineService", "LoadDefaultConfig",
                       "argv0=" + std::string(argv0 ? argv0 : ""));
    }
    RuntimeConfig config;
    config.projectRoot = std::filesystem::current_path();
    config.width = 1024;
    config.height = 768;
    config.windowTitle = "SDL3 Game Engine";
    return config;
}

}  // namespace sdl3cpp::services::impl
