#define SDL_MAIN_HANDLED

#include <CLI/CLI.hpp>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

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

struct AppOptions {
    uint32_t width = sdl3cpp::app::kWidth;
    uint32_t height = sdl3cpp::app::kHeight;
    std::filesystem::path scriptPath;
    std::optional<std::filesystem::path> configOutput;
};

AppOptions ParseCommandLine(int argc, char** argv) {
    std::filesystem::path defaultScript = FindScriptPath(argc > 0 ? argv[0] : nullptr);
    std::string scriptPathText = defaultScript.string();
    std::string configOutputText;
    uint32_t width = sdl3cpp::app::kWidth;
    uint32_t height = sdl3cpp::app::kHeight;

    CLI::App app("SDL3 + Vulkan demo CLI");
    app.add_option("-s,--script", scriptPathText, "Lua script to execute")
        ->default_str(scriptPathText)
        ->check(CLI::ExistingFile);
    app.add_option("--width", width, "Window width")
        ->default_val(std::to_string(width))
        ->check(CLI::PositiveNumber);
    app.add_option("--height", height, "Window height")
        ->default_val(std::to_string(height))
        ->check(CLI::PositiveNumber);
    app.add_option("-o,--config-out", configOutputText,
                   "Write a JSON file describing the runtime variables");
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        app.exit(e);
        throw;
    }

    AppOptions options;
    options.width = width;
    options.height = height;
    options.scriptPath = std::filesystem::absolute(std::filesystem::path(scriptPathText));
    if (!std::filesystem::exists(options.scriptPath)) {
        throw std::runtime_error("Lua script not found at " + options.scriptPath.string());
    }

    if (!configOutputText.empty()) {
        options.configOutput = std::filesystem::absolute(std::filesystem::path(configOutputText));
    }
    return options;
}

void WriteRuntimeConfigJson(const AppOptions& options, const std::filesystem::path& configPath) {
    rapidjson::Document document;
    document.SetObject();
    auto& allocator = document.GetAllocator();

    document.AddMember("window_width", options.width, allocator);
    document.AddMember("window_height", options.height, allocator);
    document.AddMember("lua_script", rapidjson::Value(options.scriptPath.string().c_str(), allocator), allocator);

    std::filesystem::path scriptsDir = options.scriptPath.parent_path();
    document.AddMember("scripts_directory",
                       rapidjson::Value(scriptsDir.string().c_str(), allocator), allocator);

    std::filesystem::path projectRoot = scriptsDir.parent_path();
    if (!projectRoot.empty()) {
        document.AddMember("project_root",
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
    document.AddMember("config_file", rapidjson::Value(configPath.string().c_str(), allocator),
                       allocator);

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
        if (options.configOutput) {
            WriteRuntimeConfigJson(options, *options.configOutput);
        }
        sdl3cpp::app::Sdl3App app(options.scriptPath);
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
