#include "json_config_writer_service.hpp"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <vulkan/vulkan.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

JsonConfigWriterService::JsonConfigWriterService(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("JsonConfigWriterService", "JsonConfigWriterService");
    }
}

void JsonConfigWriterService::WriteConfig(const RuntimeConfig& config, const std::filesystem::path& configPath) {
    if (logger_) {
        logger_->Trace("JsonConfigWriterService", "WriteConfig",
                       "config.width=" + std::to_string(config.width) +
                       ", config.height=" + std::to_string(config.height) +
                       ", config.scriptPath=" + config.scriptPath.string() +
                       ", config.luaDebug=" + std::string(config.luaDebug ? "true" : "false") +
                       ", config.windowTitle=" + config.windowTitle +
                       ", configPath=" + configPath.string(),
                       "Entering");
    }

    rapidjson::Document document;
    document.SetObject();
    auto& allocator = document.GetAllocator();

    auto addStringMember = [&](const char* name, const std::string& value) {
        rapidjson::Value nameValue(name, allocator);
        rapidjson::Value stringValue(value.c_str(), allocator);
        document.AddMember(nameValue, stringValue, allocator);
    };

    document.AddMember("window_width", config.width, allocator);
    document.AddMember("window_height", config.height, allocator);
    addStringMember("lua_script", config.scriptPath.string());

    std::filesystem::path scriptsDir = config.scriptPath.parent_path();
    addStringMember("scripts_directory", scriptsDir.string());

    std::filesystem::path projectRoot = scriptsDir.parent_path();
    if (!projectRoot.empty()) {
        addStringMember("project_root", projectRoot.string());
        addStringMember("shaders_directory", (projectRoot / "shaders").string());
    } else {
        addStringMember("shaders_directory", "shaders");
    }

    std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    rapidjson::Value extensionArray(rapidjson::kArrayType);
    for (const char* extension : deviceExtensions) {
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

    if (logger_) {
        logger_->Info("JsonConfigWriterService: Wrote runtime config to " + configPath.string());
        logger_->Trace("JsonConfigWriterService", "WriteConfig", "", "Exiting");
    }
}

}  // namespace sdl3cpp::services::impl
