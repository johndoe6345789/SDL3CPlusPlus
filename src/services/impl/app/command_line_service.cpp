#include "command_line_service.hpp"

#include "../config/json_config_service.hpp"
#include <CLI/CLI.hpp>

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

CommandLineService::CommandLineService(std::shared_ptr<ILogger> logger,
                                       std::shared_ptr<IPlatformService> platformService)
    : logger_(std::move(logger)),
      platformService_(std::move(platformService)) {
    if (!logger_) {
        throw std::runtime_error("CommandLineService requires a logger");
    }
    logger_->Trace("CommandLineService", "CommandLineService",
                   "platformService=" + std::string(platformService_ ? "set" : "null"),
                   "Created");
}

CommandLineOptions CommandLineService::Parse(int argc, char** argv) {
    std::string argv0;
    if (argc > 0 && argv && argv[0]) {
        argv0 = argv[0];
    }
    bool traceRequested = false;
    for (int i = 1; i < argc; ++i) {
        if (argv[i] && std::string(argv[i]) == "--trace") {
            traceRequested = true;
            break;
        }
    }
    if (traceRequested) {
        logger_->SetLevel(LogLevel::TRACE);
    }

    logger_->Trace("CommandLineService", "Parse",
                   "argc=" + std::to_string(argc) +
                   ", argvIsNull=" + std::string(argv ? "false" : "true") +
                   ", argv0=" + argv0,
                   "Entering");

    std::string jsonInputText;
    std::string seedOutputText;
    std::string setDefaultJsonPath;
    bool dumpRuntimeJson = false;
    bool traceRuntime = false;

    CLI::App app("SDL3 + bgfx runtime helper");
    app.add_option("-j,--json-file-in", jsonInputText, "Path to a runtime JSON config")
        ->check(CLI::ExistingFile);
    app.add_option("-s,--create-seed-json", seedOutputText,
                   "Write a template runtime JSON file");
    auto* setDefaultJsonOption = app.add_option(
        "-d,--set-default-json", setDefaultJsonPath,
        "Persist the runtime JSON to the platform default location (XDG/APPDATA); "
        "provide PATH to copy that JSON instead of using the default contents");
    setDefaultJsonOption->type_name("PATH");
    setDefaultJsonOption->type_size(1, 1);
    setDefaultJsonOption->expected(0, 1);
    app.add_flag("--dump-json", dumpRuntimeJson, "Print the runtime JSON that was loaded");
    app.add_flag("--trace", traceRuntime, "Emit a log line when key functions/methods run");

    try {
        app.parse(argc, argv);
    } catch (const CLI::CallForHelp& e) {
        std::exit(app.exit(e));
    } catch (const CLI::CallForVersion& e) {
        std::exit(app.exit(e));
    } catch (const CLI::ParseError& e) {
        app.exit(e);
        throw;
    }

    bool shouldSaveDefault = setDefaultJsonOption->count() > 0;
    std::optional<std::filesystem::path> providedDefaultPath;
    if (shouldSaveDefault && !setDefaultJsonPath.empty()) {
        providedDefaultPath = std::filesystem::absolute(setDefaultJsonPath);
    }

    RuntimeConfig runtimeConfig;
    if (!jsonInputText.empty()) {
        runtimeConfig = LoadConfigFromJson(std::filesystem::absolute(jsonInputText), dumpRuntimeJson);
    } else if (providedDefaultPath) {
        runtimeConfig = LoadConfigFromJson(*providedDefaultPath, dumpRuntimeJson);
    } else if (auto defaultPath = GetDefaultConfigPath();
               defaultPath && std::filesystem::exists(*defaultPath)) {
        runtimeConfig = LoadConfigFromJson(*defaultPath, dumpRuntimeJson);
    } else {
        runtimeConfig = LoadDefaultConfig(argc > 0 ? argv[0] : nullptr);
    }

    CommandLineOptions options;
    options.runtimeConfig = std::move(runtimeConfig);
    if (!seedOutputText.empty()) {
        options.seedOutput = std::filesystem::absolute(seedOutputText);
    }
    options.saveDefaultJson = shouldSaveDefault;
    options.dumpRuntimeJson = dumpRuntimeJson;
    options.traceEnabled = traceRuntime;

    logger_->Trace("CommandLineService", "Parse", "", "Exiting");
    return options;
}

std::optional<std::filesystem::path> CommandLineService::GetDefaultConfigPath() const {
    logger_->Trace("CommandLineService", "GetDefaultConfigPath");
    if (!platformService_) {
        logger_->Warn("CommandLineService::GetDefaultConfigPath: Platform service not available");
        return std::nullopt;
    }
    if (auto dir = platformService_->GetUserConfigDirectory()) {
        return *dir / "default_runtime.json";
    }
    return std::nullopt;
}

RuntimeConfig CommandLineService::LoadConfigFromJson(const std::filesystem::path& configPath, bool dumpConfig) {
    logger_->Trace("CommandLineService", "LoadConfigFromJson",
                   "configPath=" + configPath.string() +
                   ", dumpConfig=" + std::string(dumpConfig ? "true" : "false"));
    JsonConfigService configService(logger_, configPath, dumpConfig);
    return configService.GetConfig();
}

RuntimeConfig CommandLineService::LoadDefaultConfig(const char* argv0) {
    logger_->Trace("CommandLineService", "LoadDefaultConfig",
                   "argv0=" + std::string(argv0 ? argv0 : ""));
    JsonConfigService configService(logger_, argv0);
    return configService.GetConfig();
}

}  // namespace sdl3cpp::services::impl
