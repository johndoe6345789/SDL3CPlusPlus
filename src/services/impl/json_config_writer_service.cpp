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

    rapidjson::Value mouseGrabObject(rapidjson::kObjectType);
    mouseGrabObject.AddMember("enabled", config.mouseGrab.enabled, allocator);
    mouseGrabObject.AddMember("grab_on_click", config.mouseGrab.grabOnClick, allocator);
    mouseGrabObject.AddMember("release_on_escape", config.mouseGrab.releaseOnEscape, allocator);
    mouseGrabObject.AddMember("start_grabbed", config.mouseGrab.startGrabbed, allocator);
    mouseGrabObject.AddMember("hide_cursor", config.mouseGrab.hideCursor, allocator);
    mouseGrabObject.AddMember("relative_mode", config.mouseGrab.relativeMode, allocator);
    mouseGrabObject.AddMember("grab_mouse_button",
                              rapidjson::Value(config.mouseGrab.grabMouseButton.c_str(), allocator),
                              allocator);
    mouseGrabObject.AddMember("release_key",
                              rapidjson::Value(config.mouseGrab.releaseKey.c_str(), allocator),
                              allocator);
    document.AddMember("mouse_grab", mouseGrabObject, allocator);

    rapidjson::Value bindingsObject(rapidjson::kObjectType);
    auto addBindingMember = [&](const char* name, const std::string& value) {
        rapidjson::Value nameValue(name, allocator);
        rapidjson::Value stringValue(value.c_str(), allocator);
        bindingsObject.AddMember(nameValue, stringValue, allocator);
    };
    addBindingMember("move_forward", config.inputBindings.moveForwardKey);
    addBindingMember("move_back", config.inputBindings.moveBackKey);
    addBindingMember("move_left", config.inputBindings.moveLeftKey);
    addBindingMember("move_right", config.inputBindings.moveRightKey);
    addBindingMember("music_toggle", config.inputBindings.musicToggleKey);
    addBindingMember("music_toggle_gamepad", config.inputBindings.musicToggleGamepadButton);
    addBindingMember("gamepad_move_x_axis", config.inputBindings.gamepadMoveXAxis);
    addBindingMember("gamepad_move_y_axis", config.inputBindings.gamepadMoveYAxis);
    addBindingMember("gamepad_look_x_axis", config.inputBindings.gamepadLookXAxis);
    addBindingMember("gamepad_look_y_axis", config.inputBindings.gamepadLookYAxis);
    addBindingMember("gamepad_dpad_up", config.inputBindings.gamepadDpadUpButton);
    addBindingMember("gamepad_dpad_down", config.inputBindings.gamepadDpadDownButton);
    addBindingMember("gamepad_dpad_left", config.inputBindings.gamepadDpadLeftButton);
    addBindingMember("gamepad_dpad_right", config.inputBindings.gamepadDpadRightButton);

    auto addMappingObject = [&](const char* name,
                                const std::unordered_map<std::string, std::string>& mappings,
                                rapidjson::Value& target) {
        rapidjson::Value mappingObject(rapidjson::kObjectType);
        for (const auto& [key, value] : mappings) {
            rapidjson::Value keyValue(key.c_str(), allocator);
            rapidjson::Value stringValue(value.c_str(), allocator);
            mappingObject.AddMember(keyValue, stringValue, allocator);
        }
        target.AddMember(rapidjson::Value(name, allocator), mappingObject, allocator);
    };

    addMappingObject("gamepad_button_actions", config.inputBindings.gamepadButtonActions, bindingsObject);
    addMappingObject("gamepad_axis_actions", config.inputBindings.gamepadAxisActions, bindingsObject);
    bindingsObject.AddMember("gamepad_axis_action_threshold",
                             config.inputBindings.gamepadAxisActionThreshold, allocator);
    document.AddMember("input_bindings", bindingsObject, allocator);

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
