#include <CLI/CLI.hpp>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "app/trace.hpp"
#include "app/sdl3_app.hpp"

namespace {

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
};

RuntimeConfig GenerateDefaultRuntimeConfig(const char* argv0) {
    RuntimeConfig config;
    config.scriptPath = FindScriptPath(argv0);
    return config;
}

RuntimeConfig LoadRuntimeConfigFromJson(const std::filesystem::path& configPath, bool dumpConfig) {
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

    return config;
}

std::optional<std::filesystem::path> GetUserConfigDirectory() {
#ifdef _WIN32
    if (const char* appData = std::getenv("APPDATA")) {
        return std::filesystem::path(appData) / "sdl3cpp";
    }
#else
    if (const char* xdgConfig = std::getenv("XDG_CONFIG_HOME")) {
        return std::filesystem::path(xdgConfig) / "sdl3cpp";
    }
    if (const char* home = std::getenv("HOME")) {
        return std::filesystem::path(home) / ".config" / "sdl3cpp";
    }
#endif
    return std::nullopt;
}

std::optional<std::filesystem::path> GetDefaultConfigPath() {
    if (auto dir = GetUserConfigDirectory()) {
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

void WriteRuntimeConfigJson(const RuntimeConfig& runtimeConfig,
                            const std::filesystem::path& configPath) {
    rapidjson::Document document;
    document.SetObject();
    auto& allocator = document.GetAllocator();

    document.AddMember("window_width", runtimeConfig.width, allocator);
    document.AddMember("window_height", runtimeConfig.height, allocator);
    document.AddMember("lua_script",
                       rapidjson::Value(runtimeConfig.scriptPath.string().c_str(), allocator),
                       allocator);

    std::filesystem::path scriptsDir = runtimeConfig.scriptPath.parent_path();
    document.AddMember("scripts_directory",
                       rapidjson::Value(scriptsDir.string().c_str(), allocator), allocator);

    std::filesystem::path projectRoot = scriptsDir.parent_path();
    if (!projectRoot.empty()) {
        document.AddMember(
            "project_root",
            rapidjson::Value(projectRoot.string().c_str(), allocator), allocator);
        document.AddMember(
            "shaders_directory",
            rapidjson::Value((projectRoot / "shaders").string().c_str(), allocator), allocator);
    } else {
        document.AddMember("shaders_directory",
                           rapidjson::Value("shaders", allocator), allocator);
    }

    rapidjson::Value extensionArray(rapidjson::kArrayType);
    for (const char* extension : sdl3cpp::app::kDeviceExtensions) {
        extensionArray.PushBack(rapidjson::Value(extension, allocator), allocator);
    }
    document.AddMember("device_extensions", extensionArray, allocator);
    document.AddMember("config_file",
                       rapidjson::Value(configPath.string().c_str(), allocator), allocator);

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
    try {
        AppOptions options = ParseCommandLine(argc, argv);
        TraceLogger::SetEnabled(options.traceEnabled);
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
        sdl3cpp::app::Sdl3App app(options.runtimeConfig.scriptPath);
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
