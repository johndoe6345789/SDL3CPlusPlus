#include "json_config_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <vulkan/vulkan.h>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace sdl3cpp::services::impl {

// Default Vulkan device extensions
static const std::vector<const char*> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

JsonConfigService::JsonConfigService(std::shared_ptr<ILogger> logger, const char* argv0)
    : logger_(std::move(logger)), config_(RuntimeConfig{}) {
    if (logger_) {
        logger_->Trace("JsonConfigService", "JsonConfigService",
                       "argv0=" + std::string(argv0 ? argv0 : ""));
    }
    config_.scriptPath = FindScriptPath(argv0);
    logger_->Info("JsonConfigService initialized with default configuration");
}

JsonConfigService::JsonConfigService(std::shared_ptr<ILogger> logger, const std::filesystem::path& configPath, bool dumpConfig)
    : logger_(std::move(logger)), config_(LoadFromJson(logger_, configPath, dumpConfig)) {
    if (logger_) {
        logger_->Trace("JsonConfigService", "JsonConfigService",
                       "configPath=" + configPath.string() +
                       ", dumpConfig=" + std::string(dumpConfig ? "true" : "false"));
    }
    logger_->Info("JsonConfigService initialized from config file: " + configPath.string());
}

JsonConfigService::JsonConfigService(std::shared_ptr<ILogger> logger, const RuntimeConfig& config)
    : logger_(std::move(logger)), config_(config) {
    if (logger_) {
        logger_->Trace("JsonConfigService", "JsonConfigService",
                       "config.width=" + std::to_string(config.width) +
                       ", config.height=" + std::to_string(config.height) +
                       ", config.scriptPath=" + config.scriptPath.string() +
                       ", config.luaDebug=" + std::string(config.luaDebug ? "true" : "false") +
                       ", config.windowTitle=" + config.windowTitle);
    }
    logger_->Info("JsonConfigService initialized with explicit configuration");
}

std::vector<const char*> JsonConfigService::GetDeviceExtensions() const {
    if (logger_) {
        logger_->Trace("JsonConfigService", "GetDeviceExtensions");
    }
    return kDeviceExtensions;
}

std::filesystem::path JsonConfigService::FindScriptPath(const char* argv0) {
    if (logger_) {
        logger_->Trace("JsonConfigService", "FindScriptPath",
                       "argv0=" + std::string(argv0 ? argv0 : ""));
    }
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

RuntimeConfig JsonConfigService::LoadFromJson(std::shared_ptr<ILogger> logger, const std::filesystem::path& configPath, bool dumpConfig) {
    std::string args = "configPath=" + configPath.string() +
        ", dumpConfig=" + (dumpConfig ? "true" : "false");
    logger->Trace("JsonConfigService", "LoadFromJson", args);

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

    if (document.HasMember("window_title")) {
        const auto& value = document["window_title"];
        if (!value.IsString()) {
            throw std::runtime_error("JSON member 'window_title' must be a string");
        }
        config.windowTitle = value.GetString();
    }

    if (document.HasMember("mouse_grab")) {
        const auto& mouseGrabValue = document["mouse_grab"];
        if (!mouseGrabValue.IsObject()) {
            throw std::runtime_error("JSON member 'mouse_grab' must be an object");
        }
        auto readBool = [&](const char* name, bool& target) {
            if (!mouseGrabValue.HasMember(name)) {
                return;
            }
            const auto& value = mouseGrabValue[name];
            if (!value.IsBool()) {
                throw std::runtime_error("JSON member 'mouse_grab." + std::string(name) + "' must be a boolean");
            }
            target = value.GetBool();
        };
        auto readString = [&](const char* name, std::string& target) {
            if (!mouseGrabValue.HasMember(name)) {
                return;
            }
            const auto& value = mouseGrabValue[name];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member 'mouse_grab." + std::string(name) + "' must be a string");
            }
            target = value.GetString();
        };
        readBool("enabled", config.mouseGrab.enabled);
        readBool("grab_on_click", config.mouseGrab.grabOnClick);
        readBool("release_on_escape", config.mouseGrab.releaseOnEscape);
        readBool("start_grabbed", config.mouseGrab.startGrabbed);
        readBool("hide_cursor", config.mouseGrab.hideCursor);
        readBool("relative_mode", config.mouseGrab.relativeMode);
        readString("grab_mouse_button", config.mouseGrab.grabMouseButton);
        readString("release_key", config.mouseGrab.releaseKey);
    }

    if (document.HasMember("input_bindings")) {
        const auto& bindingsValue = document["input_bindings"];
        if (!bindingsValue.IsObject()) {
            throw std::runtime_error("JSON member 'input_bindings' must be an object");
        }
        auto readBinding = [&](const char* name, std::string& target) {
            if (!bindingsValue.HasMember(name)) {
                return;
            }
            const auto& value = bindingsValue[name];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member 'input_bindings." + std::string(name) + "' must be a string");
            }
            target = value.GetString();
        };

        readBinding("move_forward", config.inputBindings.moveForwardKey);
        readBinding("move_back", config.inputBindings.moveBackKey);
        readBinding("move_left", config.inputBindings.moveLeftKey);
        readBinding("move_right", config.inputBindings.moveRightKey);
        readBinding("music_toggle", config.inputBindings.musicToggleKey);
        readBinding("music_toggle_gamepad", config.inputBindings.musicToggleGamepadButton);
        readBinding("gamepad_move_x_axis", config.inputBindings.gamepadMoveXAxis);
        readBinding("gamepad_move_y_axis", config.inputBindings.gamepadMoveYAxis);
        readBinding("gamepad_look_x_axis", config.inputBindings.gamepadLookXAxis);
        readBinding("gamepad_look_y_axis", config.inputBindings.gamepadLookYAxis);
        readBinding("gamepad_dpad_up", config.inputBindings.gamepadDpadUpButton);
        readBinding("gamepad_dpad_down", config.inputBindings.gamepadDpadDownButton);
        readBinding("gamepad_dpad_left", config.inputBindings.gamepadDpadLeftButton);
        readBinding("gamepad_dpad_right", config.inputBindings.gamepadDpadRightButton);

        auto readMapping = [&](const char* name,
                               std::unordered_map<std::string, std::string>& target) {
            if (!bindingsValue.HasMember(name)) {
                return;
            }
            const auto& mappingValue = bindingsValue[name];
            if (!mappingValue.IsObject()) {
                throw std::runtime_error("JSON member 'input_bindings." + std::string(name) + "' must be an object");
            }
            for (auto it = mappingValue.MemberBegin(); it != mappingValue.MemberEnd(); ++it) {
                if (!it->name.IsString() || !it->value.IsString()) {
                    throw std::runtime_error("JSON member 'input_bindings." + std::string(name) +
                                             "' must contain string pairs");
                }
                std::string key = it->name.GetString();
                std::string value = it->value.GetString();
                if (value.empty()) {
                    target.erase(key);
                } else {
                    target[key] = value;
                }
            }
        };

        readMapping("gamepad_button_actions", config.inputBindings.gamepadButtonActions);
        readMapping("gamepad_axis_actions", config.inputBindings.gamepadAxisActions);

        if (bindingsValue.HasMember("gamepad_axis_action_threshold")) {
            const auto& value = bindingsValue["gamepad_axis_action_threshold"];
            if (!value.IsNumber()) {
                throw std::runtime_error("JSON member 'input_bindings.gamepad_axis_action_threshold' must be a number");
            }
            config.inputBindings.gamepadAxisActionThreshold = static_cast<float>(value.GetDouble());
        }
    }

    return config;
}

}  // namespace sdl3cpp::services::impl
