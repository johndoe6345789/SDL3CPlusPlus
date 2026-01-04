#include <CLI/CLI.hpp>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "app/service_based_app.hpp"
#include <SDL3/SDL_main.h>
#include "services/interfaces/i_logger.hpp"
#include "logging/string_utils.hpp"
#include "core/platform.hpp"

namespace sdl3cpp::app {
std::atomic<bool> g_signalReceived{false};

constexpr uint32_t kWidth = 1024;
constexpr uint32_t kHeight = 768;
const std::vector<const char*> kDeviceExtensions = {"VK_KHR_swapchain"};

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

std::filesystem::path FindScriptPath(const char* argv0) {
    std::filesystem::path executable;
    if (argv0 && *argv0 != '\0') {
        executable = std::filesystem::path(argv0);
        if (executable.is_relative()) {
            executable = std::filesystem::current_path() / executable;
        }
    } else {
        executable = std::filesystem::current_path();
    }
    executable = std::filesystem::weakly_canonical(executable);
    std::filesystem::path scriptPath = executable.parent_path() / "scripts" / "cube_logic.lua";
    if (!std::filesystem::exists(scriptPath)) {
        throw std::runtime_error("Could not find Lua script at " + scriptPath.string());
    }
    return scriptPath;
}

struct RuntimeConfig {
    uint32_t width = sdl3cpp::app::kWidth;
    uint32_t height = sdl3cpp::app::kHeight;
    std::filesystem::path scriptPath;
    bool luaDebug = false;
};

RuntimeConfig GenerateDefaultRuntimeConfig(const char* argv0) {
    RuntimeConfig config;
    config.scriptPath = FindScriptPath(argv0);
    return config;
}

RuntimeConfig LoadRuntimeConfigFromJson(const std::filesystem::path& configPath, bool dumpConfig) {
    using sdl3cpp::logging::ToString;
    sdl3cpp::logging::Logger::GetInstance().TraceFunctionWithArgs("LoadRuntimeConfigFromJson", configPath.string() + " " + ToString(dumpConfig));
    std::ifstream configStream(configPath);
    if (!configStream) {
        throw std::runtime_error("Failed to open config file: " + configPath.string());
    }

    rapidjson::IStreamWrapper inputWrapper(configStream);
    rapidjson::Document document;
    document.ParseStream(inputWrapper);
    if (document.HasParseError()) {
        throw std::runtime_error("Failed to parse JSON config at " + configPath.string());
    }
    if (!document.IsObject()) {
        throw std::runtime_error("JSON config must contain an object at the root");
    }

    if (dumpConfig) {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        writer.SetIndent(' ', 2);
        document.Accept(writer);
        std::cout << "Loaded runtime config (" << configPath << "):\n"
                  << buffer.GetString() << '\n';
    }

    const char* scriptField = "lua_script";
    if (!document.HasMember(scriptField) || !document[scriptField].IsString()) {
        throw std::runtime_error("JSON config requires a string member '" + std::string(scriptField) + "'");
    }

    std::optional<std::filesystem::path> projectRoot;
    const char* projectRootField = "project_root";
    if (document.HasMember(projectRootField) && document[projectRootField].IsString()) {
        std::filesystem::path candidate(document[projectRootField].GetString());
        if (candidate.is_absolute()) {
            projectRoot = std::filesystem::weakly_canonical(candidate);
        } else {
            projectRoot = std::filesystem::weakly_canonical(configPath.parent_path() / candidate);
        }
    }

    RuntimeConfig config;
    const auto& scriptValue = document[scriptField];
    std::filesystem::path scriptPath(scriptValue.GetString());
    if (!scriptPath.is_absolute()) {
        if (projectRoot) {
            scriptPath = *projectRoot / scriptPath;
        } else {
            scriptPath = configPath.parent_path() / scriptPath;
        }
    }
    scriptPath = std::filesystem::weakly_canonical(scriptPath);
    if (!std::filesystem::exists(scriptPath)) {
        throw std::runtime_error("Lua script not found at " + scriptPath.string());
    }
    config.scriptPath = scriptPath;

    auto parseDimension = [&](const char* name, uint32_t defaultValue) -> uint32_t {
        if (!document.HasMember(name)) {
            return defaultValue;
        }
        const auto& value = document[name];
        if (value.IsUint()) {
            return value.GetUint();
        }
        if (value.IsInt()) {
            int maybeValue = value.GetInt();
            if (maybeValue >= 0) {
                return static_cast<uint32_t>(maybeValue);
            }
        }
        throw std::runtime_error(std::string("JSON member '") + name + "' must be a non-negative integer");
    };

    config.width = parseDimension("window_width", config.width);
    config.height = parseDimension("window_height", config.height);
    if (document.HasMember("lua_debug")) {
        const auto& value = document["lua_debug"];
        if (!value.IsBool()) {
            throw std::runtime_error("JSON member 'lua_debug' must be a boolean");
        }
        config.luaDebug = value.GetBool();
    }

    return config;
}

std::optional<std::filesystem::path> GetDefaultConfigPath() {
    if (auto dir = sdl3cpp::platform::GetUserConfigDirectory()) {
        return *dir / "default_runtime.json";
    }
    return std::nullopt;
}

struct AppOptions {
    RuntimeConfig runtimeConfig;
    std::optional<std::filesystem::path> seedOutput;
    bool saveDefaultJson = false;
    bool dumpRuntimeJson = false;
    bool traceEnabled = false;
};

AppOptions ParseCommandLine(int argc, char** argv) {
    std::string jsonInputText;
    std::string seedOutputText;
    std::string setDefaultJsonPath;
    bool dumpRuntimeJson = false;
    bool traceRuntime = false;

    CLI::App app("SDL3 + Vulkan runtime helper");
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
        runtimeConfig = LoadRuntimeConfigFromJson(std::filesystem::absolute(jsonInputText), dumpRuntimeJson);
    } else if (providedDefaultPath) {
        runtimeConfig = LoadRuntimeConfigFromJson(*providedDefaultPath, dumpRuntimeJson);
    } else if (auto defaultPath = GetDefaultConfigPath();
               defaultPath && std::filesystem::exists(*defaultPath)) {
        runtimeConfig = LoadRuntimeConfigFromJson(*defaultPath, dumpRuntimeJson);
    } else {
        runtimeConfig = GenerateDefaultRuntimeConfig(argc > 0 ? argv[0] : nullptr);
    }

    AppOptions options;
    options.runtimeConfig = std::move(runtimeConfig);
    if (!seedOutputText.empty()) {
        options.seedOutput = std::filesystem::absolute(seedOutputText);
    }
    options.saveDefaultJson = shouldSaveDefault;
    options.dumpRuntimeJson = dumpRuntimeJson;
    options.traceEnabled = traceRuntime;
    return options;
}

void LogRuntimeConfig(const RuntimeConfig& config, std::shared_ptr<services::ILogger> logger) {
    if (logger) {
        logger->TraceVariable("config.width", static_cast<int>(config.width));
        logger->TraceVariable("config.height", static_cast<int>(config.height));
        logger->TraceVariable("config.scriptPath", config.scriptPath.string());
    }
}

void WriteRuntimeConfigJson(const RuntimeConfig& runtimeConfig,
                            const std::filesystem::path& configPath) {
    rapidjson::Document document;
    document.SetObject();
    auto& allocator = document.GetAllocator();

    auto addStringMember = [&](const char* name, const std::string& value) {
        rapidjson::Value nameValue(name, allocator);
        rapidjson::Value stringValue(value.c_str(), allocator);
        document.AddMember(nameValue, stringValue, allocator);
    };

    document.AddMember("window_width", runtimeConfig.width, allocator);
    document.AddMember("window_height", runtimeConfig.height, allocator);
    addStringMember("lua_script", runtimeConfig.scriptPath.string());

    std::filesystem::path scriptsDir = runtimeConfig.scriptPath.parent_path();
    addStringMember("scripts_directory", scriptsDir.string());

    std::filesystem::path projectRoot = scriptsDir.parent_path();
    if (!projectRoot.empty()) {
        addStringMember("project_root", projectRoot.string());
        addStringMember("shaders_directory", (projectRoot / "shaders").string());
    } else {
        addStringMember("shaders_directory", "shaders");
    }

    rapidjson::Value extensionArray(rapidjson::kArrayType);
    for (const char* extension : sdl3cpp::app::kDeviceExtensions) {
        rapidjson::Value extensionValue(extension, allocator);
        extensionArray.PushBack(extensionValue, allocator);
    }
    document.AddMember("device_extensions", extensionArray, allocator);
    addStringMember("config_file", configPath.string());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    auto parentDir = configPath.parent_path();
    if (!parentDir.empty()) {
        std::filesystem::create_directories(parentDir);
    }

    std::ofstream outFile(configPath);
    if (!outFile) {
        throw std::runtime_error("Failed to open config output file: " + configPath.string());
    }
    outFile << buffer.GetString();
}

} // namespace

int main(int argc, char** argv) {
    SDL_SetMainReady();
    SetupSignalHandlers();
    try {
        AppOptions options = ParseCommandLine(argc, argv);
        
        sdl3cpp::app::ServiceBasedApp app(options.runtimeConfig.scriptPath);
        
        // Configure logging
        services::LogLevel logLevel = options.traceEnabled ? services::LogLevel::TRACE : services::LogLevel::INFO;
        app.ConfigureLogging(logLevel, true, "sdl3_app.log");
        
        // Log startup information using service-based logging
        auto logger = app.GetLogger();
        if (logger) {
            logger->Info("Application starting");
            LogRuntimeConfig(options.runtimeConfig, logger);
        }
        if (options.seedOutput) {
            WriteRuntimeConfigJson(options.runtimeConfig, *options.seedOutput);
        }
        if (options.saveDefaultJson) {
            if (auto defaultPath = GetDefaultConfigPath()) {
                WriteRuntimeConfigJson(options.runtimeConfig, *defaultPath);
            } else {
                throw std::runtime_error("Unable to determine platform config directory");
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
