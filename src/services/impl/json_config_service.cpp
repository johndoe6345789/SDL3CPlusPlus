#include "json_config_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <array>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace sdl3cpp::services::impl {

JsonConfigService::JsonConfigService(std::shared_ptr<ILogger> logger, const char* argv0)
    : logger_(std::move(logger)), configJson_(), config_(RuntimeConfig{}) {
    if (logger_) {
        logger_->Trace("JsonConfigService", "JsonConfigService",
                       "argv0=" + std::string(argv0 ? argv0 : ""));
    }
    config_.scriptPath = FindScriptPath(argv0);
    configJson_ = BuildConfigJson(config_, {});
    logger_->Info("JsonConfigService initialized with default configuration");
}

JsonConfigService::JsonConfigService(std::shared_ptr<ILogger> logger, const std::filesystem::path& configPath, bool dumpConfig)
    : logger_(std::move(logger)),
      configJson_(),
      config_(LoadFromJson(logger_, configPath, dumpConfig, &configJson_)) {
    if (logger_) {
        logger_->Trace("JsonConfigService", "JsonConfigService",
                       "configPath=" + configPath.string() +
                       ", dumpConfig=" + std::string(dumpConfig ? "true" : "false"));
    }
    logger_->Info("JsonConfigService initialized from config file: " + configPath.string());
}

JsonConfigService::JsonConfigService(std::shared_ptr<ILogger> logger, const RuntimeConfig& config)
    : logger_(std::move(logger)), configJson_(BuildConfigJson(config, {})), config_(config) {
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

RuntimeConfig JsonConfigService::LoadFromJson(std::shared_ptr<ILogger> logger,
                                              const std::filesystem::path& configPath,
                                              bool dumpConfig,
                                              std::string* configJson) {
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

    if (dumpConfig || configJson) {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        writer.SetIndent(' ', 2);
        document.Accept(writer);
        if (dumpConfig) {
            std::cout << "Loaded runtime config (" << configPath << "):\n"
                      << buffer.GetString() << '\n';
        }
        if (configJson) {
            *configJson = buffer.GetString();
        }
    }

    auto getObjectMember = [&](const rapidjson::Value& parent,
                               const char* name,
                               const char* fullName) -> const rapidjson::Value* {
        if (!parent.HasMember(name)) {
            return nullptr;
        }
        const auto& value = parent[name];
        if (!value.IsObject()) {
            throw std::runtime_error("JSON member '" + std::string(fullName) + "' must be an object");
        }
        return &value;
    };

    const auto* scriptsValue = getObjectMember(document, "scripts", "scripts");
    const auto* pathsValue = getObjectMember(document, "paths", "paths");
    const auto* windowValue = getObjectMember(document, "window", "window");
    const auto* windowSizeValue = windowValue
        ? getObjectMember(*windowValue, "size", "window.size")
        : nullptr;
    const auto* inputValue = getObjectMember(document, "input", "input");
    const auto* inputBindingsValue = inputValue
        ? getObjectMember(*inputValue, "bindings", "input.bindings")
        : nullptr;
    const auto* renderingValue = getObjectMember(document, "rendering", "rendering");
    const auto* guiValue = getObjectMember(document, "gui", "gui");

    std::optional<std::string> scriptPathValue;
    if (scriptsValue && scriptsValue->HasMember("entry")) {
        const auto& value = (*scriptsValue)["entry"];
        if (!value.IsString()) {
            throw std::runtime_error("JSON member 'scripts.entry' must be a string");
        }
        scriptPathValue = value.GetString();
    } else if (document.HasMember("lua_script")) {
        const auto& value = document["lua_script"];
        if (!value.IsString()) {
            throw std::runtime_error("JSON member 'lua_script' must be a string");
        }
        scriptPathValue = value.GetString();
    }
    if (!scriptPathValue) {
        throw std::runtime_error("JSON config requires a string member 'scripts.entry' or 'lua_script'");
    }

    std::optional<std::filesystem::path> projectRoot;
    if (pathsValue && pathsValue->HasMember("project_root")) {
        const auto& value = (*pathsValue)["project_root"];
        if (!value.IsString()) {
            throw std::runtime_error("JSON member 'paths.project_root' must be a string");
        }
        std::filesystem::path candidate(value.GetString());
        if (candidate.is_absolute()) {
            projectRoot = std::filesystem::weakly_canonical(candidate);
        } else {
            projectRoot = std::filesystem::weakly_canonical(configPath.parent_path() / candidate);
        }
    } else if (document.HasMember("project_root")) {
        const auto& value = document["project_root"];
        if (!value.IsString()) {
            throw std::runtime_error("JSON member 'project_root' must be a string");
        }
        std::filesystem::path candidate(value.GetString());
        if (candidate.is_absolute()) {
            projectRoot = std::filesystem::weakly_canonical(candidate);
        } else {
            projectRoot = std::filesystem::weakly_canonical(configPath.parent_path() / candidate);
        }
    }

    RuntimeConfig config;
    std::filesystem::path scriptPath(*scriptPathValue);
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

    auto parseDimensionValue = [&](const rapidjson::Value& value, const char* name) -> uint32_t {
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

    if (windowSizeValue) {
        if (windowSizeValue->HasMember("width")) {
            config.width = parseDimensionValue((*windowSizeValue)["width"], "window.size.width");
        }
        if (windowSizeValue->HasMember("height")) {
            config.height = parseDimensionValue((*windowSizeValue)["height"], "window.size.height");
        }
    } else {
        config.width = parseDimension("window_width", config.width);
        config.height = parseDimension("window_height", config.height);
    }

    if (scriptsValue && scriptsValue->HasMember("lua_debug")) {
        const auto& value = (*scriptsValue)["lua_debug"];
        if (!value.IsBool()) {
            throw std::runtime_error("JSON member 'scripts.lua_debug' must be a boolean");
        }
        config.luaDebug = value.GetBool();
    } else if (document.HasMember("lua_debug")) {
        const auto& value = document["lua_debug"];
        if (!value.IsBool()) {
            throw std::runtime_error("JSON member 'lua_debug' must be a boolean");
        }
        config.luaDebug = value.GetBool();
    }

    if (windowValue && windowValue->HasMember("title")) {
        const auto& value = (*windowValue)["title"];
        if (!value.IsString()) {
            throw std::runtime_error("JSON member 'window.title' must be a string");
        }
        config.windowTitle = value.GetString();
    } else if (document.HasMember("window_title")) {
        const auto& value = document["window_title"];
        if (!value.IsString()) {
            throw std::runtime_error("JSON member 'window_title' must be a string");
        }
        config.windowTitle = value.GetString();
    }

    const rapidjson::Value* mouseGrabValue = nullptr;
    std::string mouseGrabPath = "mouse_grab";
    if (windowValue && windowValue->HasMember("mouse_grab")) {
        const auto& value = (*windowValue)["mouse_grab"];
        if (!value.IsObject()) {
            throw std::runtime_error("JSON member 'window.mouse_grab' must be an object");
        }
        mouseGrabValue = &value;
        mouseGrabPath = "window.mouse_grab";
    } else if (document.HasMember("mouse_grab")) {
        const auto& value = document["mouse_grab"];
        if (!value.IsObject()) {
            throw std::runtime_error("JSON member 'mouse_grab' must be an object");
        }
        mouseGrabValue = &value;
    }
    if (mouseGrabValue) {
        auto readBool = [&](const char* name, bool& target) {
            if (!mouseGrabValue->HasMember(name)) {
                return;
            }
            const auto& value = (*mouseGrabValue)[name];
            if (!value.IsBool()) {
                throw std::runtime_error("JSON member '" + mouseGrabPath + "." + std::string(name) + "' must be a boolean");
            }
            target = value.GetBool();
        };
        auto readString = [&](const char* name, std::string& target) {
            if (!mouseGrabValue->HasMember(name)) {
                return;
            }
            const auto& value = (*mouseGrabValue)[name];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member '" + mouseGrabPath + "." + std::string(name) + "' must be a string");
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

    const rapidjson::Value* bindingsValue = nullptr;
    std::string bindingsPath = "input_bindings";
    if (inputBindingsValue) {
        bindingsValue = inputBindingsValue;
        bindingsPath = "input.bindings";
    } else if (document.HasMember("input_bindings")) {
        const auto& value = document["input_bindings"];
        if (!value.IsObject()) {
            throw std::runtime_error("JSON member 'input_bindings' must be an object");
        }
        bindingsValue = &value;
    }
    if (bindingsValue) {
        struct BindingSpec {
            const char* name;
            std::string InputBindings::* member;
        };
        const std::array<BindingSpec, 18> bindingSpecs = {{
            {"move_forward", &InputBindings::moveForwardKey},
            {"move_back", &InputBindings::moveBackKey},
            {"move_left", &InputBindings::moveLeftKey},
            {"move_right", &InputBindings::moveRightKey},
            {"fly_up", &InputBindings::flyUpKey},
            {"fly_down", &InputBindings::flyDownKey},
            {"jump", &InputBindings::jumpKey},
            {"noclip_toggle", &InputBindings::noclipToggleKey},
            {"music_toggle", &InputBindings::musicToggleKey},
            {"music_toggle_gamepad", &InputBindings::musicToggleGamepadButton},
            {"gamepad_move_x_axis", &InputBindings::gamepadMoveXAxis},
            {"gamepad_move_y_axis", &InputBindings::gamepadMoveYAxis},
            {"gamepad_look_x_axis", &InputBindings::gamepadLookXAxis},
            {"gamepad_look_y_axis", &InputBindings::gamepadLookYAxis},
            {"gamepad_dpad_up", &InputBindings::gamepadDpadUpButton},
            {"gamepad_dpad_down", &InputBindings::gamepadDpadDownButton},
            {"gamepad_dpad_left", &InputBindings::gamepadDpadLeftButton},
            {"gamepad_dpad_right", &InputBindings::gamepadDpadRightButton},
        }};

        auto readBinding = [&](const BindingSpec& spec) {
            if (!bindingsValue->HasMember(spec.name)) {
                return;
            }
            const auto& value = (*bindingsValue)[spec.name];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member '" + bindingsPath + "." + std::string(spec.name) + "' must be a string");
            }
            config.inputBindings.*(spec.member) = value.GetString();
        };

        for (const auto& spec : bindingSpecs) {
            readBinding(spec);
        }

        auto readMapping = [&](const char* name,
                               std::unordered_map<std::string, std::string>& target) {
            if (!bindingsValue->HasMember(name)) {
                return;
            }
            const auto& mappingValue = (*bindingsValue)[name];
            if (!mappingValue.IsObject()) {
                throw std::runtime_error("JSON member '" + bindingsPath + "." + std::string(name) + "' must be an object");
            }
            for (auto it = mappingValue.MemberBegin(); it != mappingValue.MemberEnd(); ++it) {
                if (!it->name.IsString() || !it->value.IsString()) {
                    throw std::runtime_error("JSON member '" + bindingsPath + "." + std::string(name) +
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

        if (bindingsValue->HasMember("gamepad_axis_action_threshold")) {
            const auto& value = (*bindingsValue)["gamepad_axis_action_threshold"];
            if (!value.IsNumber()) {
                throw std::runtime_error("JSON member '" + bindingsPath + ".gamepad_axis_action_threshold' must be a number");
            }
            config.inputBindings.gamepadAxisActionThreshold = static_cast<float>(value.GetDouble());
        }
    }

    const rapidjson::Value* atmosphericsValue = nullptr;
    std::string atmosphericsPath = "atmospherics";
    if (renderingValue) {
        atmosphericsValue = getObjectMember(*renderingValue, "atmospherics", "rendering.atmospherics");
        if (atmosphericsValue) {
            atmosphericsPath = "rendering.atmospherics";
        }
    }
    if (!atmosphericsValue && document.HasMember("atmospherics")) {
        const auto& value = document["atmospherics"];
        if (!value.IsObject()) {
            throw std::runtime_error("JSON member 'atmospherics' must be an object");
        }
        atmosphericsValue = &value;
    }
    if (atmosphericsValue) {
        
        auto readFloat = [&](const char* name, float& target) {
            if (!atmosphericsValue->HasMember(name)) {
                return;
            }
            const auto& value = (*atmosphericsValue)[name];
            if (!value.IsNumber()) {
                throw std::runtime_error("JSON member '" + atmosphericsPath + "." + std::string(name) +
                                         "' must be a number");
            }
            target = static_cast<float>(value.GetDouble());
        };
        
        auto readBool = [&](const char* name, bool& target) {
            if (!atmosphericsValue->HasMember(name)) {
                return;
            }
            const auto& value = (*atmosphericsValue)[name];
            if (!value.IsBool()) {
                throw std::runtime_error("JSON member '" + atmosphericsPath + "." + std::string(name) +
                                         "' must be a boolean");
            }
            target = value.GetBool();
        };
        
        auto readFloatArray3 = [&](const char* name, std::array<float, 3>& target) {
            if (!atmosphericsValue->HasMember(name)) {
                return;
            }
            const auto& value = (*atmosphericsValue)[name];
            if (!value.IsArray() || value.Size() != 3) {
                throw std::runtime_error("JSON member '" + atmosphericsPath + "." + std::string(name) +
                                         "' must be an array of 3 numbers");
            }
            for (rapidjson::SizeType i = 0; i < 3; ++i) {
                if (!value[i].IsNumber()) {
                    throw std::runtime_error("JSON member '" + atmosphericsPath + "." + std::string(name) +
                                             "[" + std::to_string(i) + "]' must be a number");
                }
                target[i] = static_cast<float>(value[i].GetDouble());
            }
        };
        
        readFloat("ambient_strength", config.atmospherics.ambientStrength);
        readFloat("fog_density", config.atmospherics.fogDensity);
        readFloatArray3("fog_color", config.atmospherics.fogColor);
        readFloatArray3("sky_color", config.atmospherics.skyColor);
        readFloat("gamma", config.atmospherics.gamma);
        readFloat("exposure", config.atmospherics.exposure);
        readBool("enable_tone_mapping", config.atmospherics.enableToneMapping);
        readBool("enable_shadows", config.atmospherics.enableShadows);
        readBool("enable_ssgi", config.atmospherics.enableSSGI);
        readBool("enable_volumetric_lighting", config.atmospherics.enableVolumetricLighting);
        readFloat("pbr_roughness", config.atmospherics.pbrRoughness);
        readFloat("pbr_metallic", config.atmospherics.pbrMetallic);
    }

    const rapidjson::Value* bgfxValue = nullptr;
    std::string bgfxPath = "bgfx";
    if (renderingValue) {
        bgfxValue = getObjectMember(*renderingValue, "bgfx", "rendering.bgfx");
        if (bgfxValue) {
            bgfxPath = "rendering.bgfx";
        }
    }
    if (!bgfxValue && document.HasMember("bgfx")) {
        const auto& value = document["bgfx"];
        if (!value.IsObject()) {
            throw std::runtime_error("JSON member 'bgfx' must be an object");
        }
        bgfxValue = &value;
    }
    if (bgfxValue) {
        if (bgfxValue->HasMember("renderer")) {
            const auto& value = (*bgfxValue)["renderer"];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member '" + bgfxPath + ".renderer' must be a string");
            }
            config.bgfx.renderer = value.GetString();
        }
    }

    const rapidjson::Value* materialValue = nullptr;
    std::string materialPath = "materialx";
    if (renderingValue) {
        materialValue = getObjectMember(*renderingValue, "materialx", "rendering.materialx");
        if (materialValue) {
            materialPath = "rendering.materialx";
        }
    }
    if (!materialValue && document.HasMember("materialx")) {
        const auto& value = document["materialx"];
        if (!value.IsObject()) {
            throw std::runtime_error("JSON member 'materialx' must be an object");
        }
        materialValue = &value;
    }

    bool materialShaderKeyProvided = false;
    if (materialValue) {
        if (materialValue->HasMember("enabled")) {
            const auto& value = (*materialValue)["enabled"];
            if (!value.IsBool()) {
                throw std::runtime_error("JSON member '" + materialPath + ".enabled' must be a boolean");
            }
            config.materialX.enabled = value.GetBool();
        }
        if (materialValue->HasMember("document")) {
            const auto& value = (*materialValue)["document"];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member '" + materialPath + ".document' must be a string");
            }
            config.materialX.documentPath = value.GetString();
        }
        if (materialValue->HasMember("shader_key")) {
            const auto& value = (*materialValue)["shader_key"];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member '" + materialPath + ".shader_key' must be a string");
            }
            config.materialX.shaderKey = value.GetString();
            materialShaderKeyProvided = true;
        }
        if (materialValue->HasMember("material")) {
            const auto& value = (*materialValue)["material"];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member '" + materialPath + ".material' must be a string");
            }
            config.materialX.materialName = value.GetString();
        }
        if (materialValue->HasMember("library_path")) {
            const auto& value = (*materialValue)["library_path"];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member '" + materialPath + ".library_path' must be a string");
            }
            config.materialX.libraryPath = value.GetString();
        }
        if (materialValue->HasMember("library_folders")) {
            const auto& value = (*materialValue)["library_folders"];
            if (!value.IsArray()) {
                throw std::runtime_error("JSON member '" + materialPath + ".library_folders' must be an array");
            }
            config.materialX.libraryFolders.clear();
            for (rapidjson::SizeType i = 0; i < value.Size(); ++i) {
                if (!value[i].IsString()) {
                    throw std::runtime_error("JSON member '" + materialPath + ".library_folders[" +
                                             std::to_string(i) + "]' must be a string");
                }
                config.materialX.libraryFolders.emplace_back(value[i].GetString());
            }
        }
        if (materialValue->HasMember("use_constant_color")) {
            const auto& value = (*materialValue)["use_constant_color"];
            if (!value.IsBool()) {
                throw std::runtime_error("JSON member '" + materialPath + ".use_constant_color' must be a boolean");
            }
            config.materialX.useConstantColor = value.GetBool();
        }
        if (materialValue->HasMember("constant_color")) {
            const auto& value = (*materialValue)["constant_color"];
            if (!value.IsArray() || value.Size() != 3) {
                throw std::runtime_error("JSON member '" + materialPath + ".constant_color' must be an array of 3 numbers");
            }
            for (rapidjson::SizeType i = 0; i < 3; ++i) {
                if (!value[i].IsNumber()) {
                    throw std::runtime_error("JSON member '" + materialPath + ".constant_color[" +
                                             std::to_string(i) + "]' must be a number");
                }
                config.materialX.constantColor[i] = static_cast<float>(value[i].GetDouble());
            }
        }
    }

    const rapidjson::Value* materialsValue = nullptr;
    std::string materialsPath = "materialx_materials";
    if (materialValue && materialValue->HasMember("materials")) {
        const auto& value = (*materialValue)["materials"];
        if (!value.IsArray()) {
            throw std::runtime_error("JSON member '" + materialPath + ".materials' must be an array");
        }
        materialsValue = &value;
        materialsPath = materialPath + ".materials";
    }
    if (!materialsValue && document.HasMember("materialx_materials")) {
        const auto& value = document["materialx_materials"];
        if (!value.IsArray()) {
            throw std::runtime_error("JSON member 'materialx_materials' must be an array");
        }
        materialsValue = &value;
    }
    if (materialsValue) {
        config.materialXMaterials.clear();
        for (rapidjson::SizeType i = 0; i < materialsValue->Size(); ++i) {
            const auto& entry = (*materialsValue)[i];
            if (!entry.IsObject()) {
                throw std::runtime_error("JSON member '" + materialsPath + "[" + std::to_string(i) +
                                         "]' must be an object");
            }
            MaterialXMaterialConfig materialConfig;
            if (entry.HasMember("enabled")) {
                const auto& value = entry["enabled"];
                if (!value.IsBool()) {
                    throw std::runtime_error("JSON member '" + materialsPath + "[" + std::to_string(i) +
                                             "].enabled' must be a boolean");
                }
                materialConfig.enabled = value.GetBool();
            }
            if (entry.HasMember("document")) {
                const auto& value = entry["document"];
                if (!value.IsString()) {
                    throw std::runtime_error("JSON member '" + materialsPath + "[" + std::to_string(i) +
                                             "].document' must be a string");
                }
                materialConfig.documentPath = value.GetString();
            }
            if (entry.HasMember("shader_key")) {
                const auto& value = entry["shader_key"];
                if (!value.IsString()) {
                    throw std::runtime_error("JSON member '" + materialsPath + "[" + std::to_string(i) +
                                             "].shader_key' must be a string");
                }
                materialConfig.shaderKey = value.GetString();
            }
            if (entry.HasMember("material")) {
                const auto& value = entry["material"];
                if (!value.IsString()) {
                    throw std::runtime_error("JSON member '" + materialsPath + "[" + std::to_string(i) +
                                             "].material' must be a string");
                }
                materialConfig.materialName = value.GetString();
            }
            if (entry.HasMember("use_constant_color")) {
                const auto& value = entry["use_constant_color"];
                if (!value.IsBool()) {
                    throw std::runtime_error("JSON member '" + materialsPath + "[" + std::to_string(i) +
                                             "].use_constant_color' must be a boolean");
                }
                materialConfig.useConstantColor = value.GetBool();
            }
            if (entry.HasMember("constant_color")) {
                const auto& value = entry["constant_color"];
                if (!value.IsArray() || value.Size() != 3) {
                    throw std::runtime_error("JSON member '" + materialsPath + "[" + std::to_string(i) +
                                             "].constant_color' must be an array of 3 numbers");
                }
                for (rapidjson::SizeType channel = 0; channel < 3; ++channel) {
                    if (!value[channel].IsNumber()) {
                        throw std::runtime_error("JSON member '" + materialsPath + "[" + std::to_string(i) +
                                                 "].constant_color[" + std::to_string(channel) +
                                                 "]' must be a number");
                    }
                    materialConfig.constantColor[channel] = static_cast<float>(value[channel].GetDouble());
                }
            }

            if (materialConfig.shaderKey.empty()) {
                throw std::runtime_error("JSON member '" + materialsPath + "[" + std::to_string(i) +
                                         "].shader_key' must be provided");
            }
            if (materialConfig.documentPath.empty() && !materialConfig.useConstantColor) {
                throw std::runtime_error("JSON member '" + materialsPath + "[" + std::to_string(i) +
                                         "].document' is required when use_constant_color is false");
            }

            config.materialXMaterials.push_back(std::move(materialConfig));
        }
    }

    if (!materialShaderKeyProvided && !config.materialXMaterials.empty()) {
        config.materialX.shaderKey = config.materialXMaterials.front().shaderKey;
        if (logger) {
            logger->Trace("JsonConfigService", "LoadFromJson",
                          "materialx.shader_key not set; defaulting to first materialx_materials key=" +
                              config.materialX.shaderKey);
        }
    }

    const rapidjson::Value* guiFontValue = nullptr;
    std::string guiFontPath = "gui_font";
    if (guiValue && guiValue->HasMember("font")) {
        const auto& value = (*guiValue)["font"];
        if (!value.IsObject()) {
            throw std::runtime_error("JSON member 'gui.font' must be an object");
        }
        guiFontValue = &value;
        guiFontPath = "gui.font";
    } else if (document.HasMember("gui_font")) {
        const auto& value = document["gui_font"];
        if (!value.IsObject()) {
            throw std::runtime_error("JSON member 'gui_font' must be an object");
        }
        guiFontValue = &value;
    }
    if (guiFontValue) {
        if (guiFontValue->HasMember("use_freetype")) {
            const auto& value = (*guiFontValue)["use_freetype"];
            if (!value.IsBool()) {
                throw std::runtime_error("JSON member '" + guiFontPath + ".use_freetype' must be a boolean");
            }
            config.guiFont.useFreeType = value.GetBool();
        }
        if (guiFontValue->HasMember("font_path")) {
            const auto& value = (*guiFontValue)["font_path"];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member '" + guiFontPath + ".font_path' must be a string");
            }
            config.guiFont.fontPath = value.GetString();
        }
        if (guiFontValue->HasMember("font_size")) {
            const auto& value = (*guiFontValue)["font_size"];
            if (!value.IsNumber()) {
                throw std::runtime_error("JSON member '" + guiFontPath + ".font_size' must be a number");
            }
            config.guiFont.fontSize = static_cast<float>(value.GetDouble());
        }
    }

    if (guiValue && guiValue->HasMember("opacity")) {
        const auto& value = (*guiValue)["opacity"];
        if (!value.IsNumber()) {
            throw std::runtime_error("JSON member 'gui.opacity' must be a number");
        }
        config.guiOpacity = static_cast<float>(value.GetDouble());
    } else if (document.HasMember("gui_opacity")) {
        const auto& value = document["gui_opacity"];
        if (!value.IsNumber()) {
            throw std::runtime_error("JSON member 'gui_opacity' must be a number");
        }
        config.guiOpacity = static_cast<float>(value.GetDouble());
    }

    return config;
}

std::string JsonConfigService::BuildConfigJson(const RuntimeConfig& config,
                                               const std::filesystem::path& configPath) {
    rapidjson::Document document;
    document.SetObject();
    auto& allocator = document.GetAllocator();

    auto addStringMember = [&](rapidjson::Value& target, const char* name, const std::string& value) {
        rapidjson::Value nameValue(name, allocator);
        rapidjson::Value stringValue(value.c_str(), allocator);
        target.AddMember(nameValue, stringValue, allocator);
    };

    document.AddMember("schema_version", 2, allocator);

    rapidjson::Value scriptsObject(rapidjson::kObjectType);
    addStringMember(scriptsObject, "entry", config.scriptPath.string());
    scriptsObject.AddMember("lua_debug", config.luaDebug, allocator);
    document.AddMember("scripts", scriptsObject, allocator);

    rapidjson::Value windowObject(rapidjson::kObjectType);
    addStringMember(windowObject, "title", config.windowTitle);
    rapidjson::Value sizeObject(rapidjson::kObjectType);
    sizeObject.AddMember("width", config.width, allocator);
    sizeObject.AddMember("height", config.height, allocator);
    windowObject.AddMember("size", sizeObject, allocator);

    std::filesystem::path scriptsDir = config.scriptPath.parent_path();

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
    windowObject.AddMember("mouse_grab", mouseGrabObject, allocator);
    document.AddMember("window", windowObject, allocator);

    rapidjson::Value bgfxObject(rapidjson::kObjectType);
    bgfxObject.AddMember("renderer",
                         rapidjson::Value(config.bgfx.renderer.c_str(), allocator),
                         allocator);

    rapidjson::Value materialObject(rapidjson::kObjectType);
    materialObject.AddMember("enabled", config.materialX.enabled, allocator);
    materialObject.AddMember("document",
                             rapidjson::Value(config.materialX.documentPath.string().c_str(), allocator),
                             allocator);
    materialObject.AddMember("shader_key",
                             rapidjson::Value(config.materialX.shaderKey.c_str(), allocator),
                             allocator);
    materialObject.AddMember("material",
                             rapidjson::Value(config.materialX.materialName.c_str(), allocator),
                             allocator);
    materialObject.AddMember("library_path",
                             rapidjson::Value(config.materialX.libraryPath.string().c_str(), allocator),
                             allocator);
    rapidjson::Value libraryFolders(rapidjson::kArrayType);
    for (const auto& folder : config.materialX.libraryFolders) {
        libraryFolders.PushBack(rapidjson::Value(folder.c_str(), allocator), allocator);
    }
    materialObject.AddMember("library_folders", libraryFolders, allocator);
    materialObject.AddMember("use_constant_color", config.materialX.useConstantColor, allocator);
    rapidjson::Value constantColor(rapidjson::kArrayType);
    constantColor.PushBack(config.materialX.constantColor[0], allocator);
    constantColor.PushBack(config.materialX.constantColor[1], allocator);
    constantColor.PushBack(config.materialX.constantColor[2], allocator);
    materialObject.AddMember("constant_color", constantColor, allocator);

    if (!config.materialXMaterials.empty()) {
        rapidjson::Value materialsArray(rapidjson::kArrayType);
        for (const auto& material : config.materialXMaterials) {
            rapidjson::Value entry(rapidjson::kObjectType);
            entry.AddMember("enabled", material.enabled, allocator);
            entry.AddMember("document",
                            rapidjson::Value(material.documentPath.string().c_str(), allocator),
                            allocator);
            entry.AddMember("shader_key",
                            rapidjson::Value(material.shaderKey.c_str(), allocator),
                            allocator);
            entry.AddMember("material",
                            rapidjson::Value(material.materialName.c_str(), allocator),
                            allocator);
            entry.AddMember("use_constant_color", material.useConstantColor, allocator);
            rapidjson::Value materialColor(rapidjson::kArrayType);
            materialColor.PushBack(material.constantColor[0], allocator);
            materialColor.PushBack(material.constantColor[1], allocator);
            materialColor.PushBack(material.constantColor[2], allocator);
            entry.AddMember("constant_color", materialColor, allocator);
            materialsArray.PushBack(entry, allocator);
        }
        materialObject.AddMember("materials", materialsArray, allocator);
    }

    rapidjson::Value renderingObject(rapidjson::kObjectType);
    renderingObject.AddMember("bgfx", bgfxObject, allocator);
    renderingObject.AddMember("materialx", materialObject, allocator);

    rapidjson::Value atmosphericsObject(rapidjson::kObjectType);
    atmosphericsObject.AddMember("ambient_strength", config.atmospherics.ambientStrength, allocator);
    atmosphericsObject.AddMember("fog_density", config.atmospherics.fogDensity, allocator);
    rapidjson::Value fogColor(rapidjson::kArrayType);
    fogColor.PushBack(config.atmospherics.fogColor[0], allocator);
    fogColor.PushBack(config.atmospherics.fogColor[1], allocator);
    fogColor.PushBack(config.atmospherics.fogColor[2], allocator);
    atmosphericsObject.AddMember("fog_color", fogColor, allocator);
    rapidjson::Value skyColor(rapidjson::kArrayType);
    skyColor.PushBack(config.atmospherics.skyColor[0], allocator);
    skyColor.PushBack(config.atmospherics.skyColor[1], allocator);
    skyColor.PushBack(config.atmospherics.skyColor[2], allocator);
    atmosphericsObject.AddMember("sky_color", skyColor, allocator);
    atmosphericsObject.AddMember("gamma", config.atmospherics.gamma, allocator);
    atmosphericsObject.AddMember("exposure", config.atmospherics.exposure, allocator);
    atmosphericsObject.AddMember("enable_tone_mapping", config.atmospherics.enableToneMapping, allocator);
    atmosphericsObject.AddMember("enable_shadows", config.atmospherics.enableShadows, allocator);
    atmosphericsObject.AddMember("enable_ssgi", config.atmospherics.enableSSGI, allocator);
    atmosphericsObject.AddMember("enable_volumetric_lighting", config.atmospherics.enableVolumetricLighting, allocator);
    atmosphericsObject.AddMember("pbr_roughness", config.atmospherics.pbrRoughness, allocator);
    atmosphericsObject.AddMember("pbr_metallic", config.atmospherics.pbrMetallic, allocator);
    renderingObject.AddMember("atmospherics", atmosphericsObject, allocator);
    document.AddMember("rendering", renderingObject, allocator);

    rapidjson::Value bindingsObject(rapidjson::kObjectType);
    auto addBindingMember = [&](const char* name, const std::string& value) {
        rapidjson::Value nameValue(name, allocator);
        rapidjson::Value stringValue(value.c_str(), allocator);
        bindingsObject.AddMember(nameValue, stringValue, allocator);
    };
    struct BindingSpec {
        const char* name;
        std::string InputBindings::* member;
    };
    const std::array<BindingSpec, 18> bindingSpecs = {{
        {"move_forward", &InputBindings::moveForwardKey},
        {"move_back", &InputBindings::moveBackKey},
        {"move_left", &InputBindings::moveLeftKey},
        {"move_right", &InputBindings::moveRightKey},
        {"fly_up", &InputBindings::flyUpKey},
        {"fly_down", &InputBindings::flyDownKey},
        {"jump", &InputBindings::jumpKey},
        {"noclip_toggle", &InputBindings::noclipToggleKey},
        {"music_toggle", &InputBindings::musicToggleKey},
        {"music_toggle_gamepad", &InputBindings::musicToggleGamepadButton},
        {"gamepad_move_x_axis", &InputBindings::gamepadMoveXAxis},
        {"gamepad_move_y_axis", &InputBindings::gamepadMoveYAxis},
        {"gamepad_look_x_axis", &InputBindings::gamepadLookXAxis},
        {"gamepad_look_y_axis", &InputBindings::gamepadLookYAxis},
        {"gamepad_dpad_up", &InputBindings::gamepadDpadUpButton},
        {"gamepad_dpad_down", &InputBindings::gamepadDpadDownButton},
        {"gamepad_dpad_left", &InputBindings::gamepadDpadLeftButton},
        {"gamepad_dpad_right", &InputBindings::gamepadDpadRightButton},
    }};
    for (const auto& spec : bindingSpecs) {
        addBindingMember(spec.name, config.inputBindings.*(spec.member));
    }

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
    rapidjson::Value inputObject(rapidjson::kObjectType);
    inputObject.AddMember("bindings", bindingsObject, allocator);
    document.AddMember("input", inputObject, allocator);

    std::filesystem::path projectRoot = scriptsDir.parent_path();
    rapidjson::Value pathsObject(rapidjson::kObjectType);
    if (!scriptsDir.empty()) {
        addStringMember(pathsObject, "scripts", scriptsDir.string());
    }
    if (!projectRoot.empty()) {
        addStringMember(pathsObject, "project_root", projectRoot.string());
        addStringMember(pathsObject, "shaders", (projectRoot / "shaders").string());
    } else {
        addStringMember(pathsObject, "shaders", "shaders");
    }
    document.AddMember("paths", pathsObject, allocator);

    rapidjson::Value guiObject(rapidjson::kObjectType);
    rapidjson::Value fontObject(rapidjson::kObjectType);
    fontObject.AddMember("use_freetype", config.guiFont.useFreeType, allocator);
    fontObject.AddMember("font_path",
                         rapidjson::Value(config.guiFont.fontPath.string().c_str(), allocator),
                         allocator);
    fontObject.AddMember("font_size", config.guiFont.fontSize, allocator);
    guiObject.AddMember("font", fontObject, allocator);
    guiObject.AddMember("opacity", config.guiOpacity, allocator);
    document.AddMember("gui", guiObject, allocator);

    if (!configPath.empty()) {
        addStringMember(document, "config_file", configPath.string());
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    return buffer.GetString();
}

}  // namespace sdl3cpp::services::impl
