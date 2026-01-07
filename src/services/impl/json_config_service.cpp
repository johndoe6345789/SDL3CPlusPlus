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
            if (!bindingsValue.HasMember(spec.name)) {
                return;
            }
            const auto& value = bindingsValue[spec.name];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member 'input_bindings." + std::string(spec.name) + "' must be a string");
            }
            config.inputBindings.*(spec.member) = value.GetString();
        };

        for (const auto& spec : bindingSpecs) {
            readBinding(spec);
        }

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

    if (document.HasMember("atmospherics")) {
        const auto& atmosphericsValue = document["atmospherics"];
        if (!atmosphericsValue.IsObject()) {
            throw std::runtime_error("JSON member 'atmospherics' must be an object");
        }
        
        auto readFloat = [&](const char* name, float& target) {
            if (!atmosphericsValue.HasMember(name)) {
                return;
            }
            const auto& value = atmosphericsValue[name];
            if (!value.IsNumber()) {
                throw std::runtime_error("JSON member 'atmospherics." + std::string(name) + "' must be a number");
            }
            target = static_cast<float>(value.GetDouble());
        };
        
        auto readBool = [&](const char* name, bool& target) {
            if (!atmosphericsValue.HasMember(name)) {
                return;
            }
            const auto& value = atmosphericsValue[name];
            if (!value.IsBool()) {
                throw std::runtime_error("JSON member 'atmospherics." + std::string(name) + "' must be a boolean");
            }
            target = value.GetBool();
        };
        
        auto readFloatArray3 = [&](const char* name, std::array<float, 3>& target) {
            if (!atmosphericsValue.HasMember(name)) {
                return;
            }
            const auto& value = atmosphericsValue[name];
            if (!value.IsArray() || value.Size() != 3) {
                throw std::runtime_error("JSON member 'atmospherics." + std::string(name) + "' must be an array of 3 numbers");
            }
            for (rapidjson::SizeType i = 0; i < 3; ++i) {
                if (!value[i].IsNumber()) {
                    throw std::runtime_error("JSON member 'atmospherics." + std::string(name) + "[" + std::to_string(i) + "]' must be a number");
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

    if (document.HasMember("bgfx")) {
        const auto& bgfxValue = document["bgfx"];
        if (!bgfxValue.IsObject()) {
            throw std::runtime_error("JSON member 'bgfx' must be an object");
        }
        if (bgfxValue.HasMember("renderer")) {
            const auto& value = bgfxValue["renderer"];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member 'bgfx.renderer' must be a string");
            }
            config.bgfx.renderer = value.GetString();
        }
    }

    bool materialShaderKeyProvided = false;
    if (document.HasMember("materialx")) {
        const auto& materialValue = document["materialx"];
        if (!materialValue.IsObject()) {
            throw std::runtime_error("JSON member 'materialx' must be an object");
        }
        if (materialValue.HasMember("enabled")) {
            const auto& value = materialValue["enabled"];
            if (!value.IsBool()) {
                throw std::runtime_error("JSON member 'materialx.enabled' must be a boolean");
            }
            config.materialX.enabled = value.GetBool();
        }
        if (materialValue.HasMember("document")) {
            const auto& value = materialValue["document"];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member 'materialx.document' must be a string");
            }
            config.materialX.documentPath = value.GetString();
        }
        if (materialValue.HasMember("shader_key")) {
            const auto& value = materialValue["shader_key"];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member 'materialx.shader_key' must be a string");
            }
            config.materialX.shaderKey = value.GetString();
            materialShaderKeyProvided = true;
        }
        if (materialValue.HasMember("material")) {
            const auto& value = materialValue["material"];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member 'materialx.material' must be a string");
            }
            config.materialX.materialName = value.GetString();
        }
        if (materialValue.HasMember("library_path")) {
            const auto& value = materialValue["library_path"];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member 'materialx.library_path' must be a string");
            }
            config.materialX.libraryPath = value.GetString();
        }
        if (materialValue.HasMember("library_folders")) {
            const auto& value = materialValue["library_folders"];
            if (!value.IsArray()) {
                throw std::runtime_error("JSON member 'materialx.library_folders' must be an array");
            }
            config.materialX.libraryFolders.clear();
            for (rapidjson::SizeType i = 0; i < value.Size(); ++i) {
                if (!value[i].IsString()) {
                    throw std::runtime_error("JSON member 'materialx.library_folders[" + std::to_string(i) + "]' must be a string");
                }
                config.materialX.libraryFolders.emplace_back(value[i].GetString());
            }
        }
        if (materialValue.HasMember("use_constant_color")) {
            const auto& value = materialValue["use_constant_color"];
            if (!value.IsBool()) {
                throw std::runtime_error("JSON member 'materialx.use_constant_color' must be a boolean");
            }
            config.materialX.useConstantColor = value.GetBool();
        }
        if (materialValue.HasMember("constant_color")) {
            const auto& value = materialValue["constant_color"];
            if (!value.IsArray() || value.Size() != 3) {
                throw std::runtime_error("JSON member 'materialx.constant_color' must be an array of 3 numbers");
            }
            for (rapidjson::SizeType i = 0; i < 3; ++i) {
                if (!value[i].IsNumber()) {
                    throw std::runtime_error("JSON member 'materialx.constant_color[" + std::to_string(i) + "]' must be a number");
                }
                config.materialX.constantColor[i] = static_cast<float>(value[i].GetDouble());
            }
        }
    }

    if (document.HasMember("materialx_materials")) {
        const auto& materialsValue = document["materialx_materials"];
        if (!materialsValue.IsArray()) {
            throw std::runtime_error("JSON member 'materialx_materials' must be an array");
        }
        config.materialXMaterials.clear();
        for (rapidjson::SizeType i = 0; i < materialsValue.Size(); ++i) {
            const auto& entry = materialsValue[i];
            if (!entry.IsObject()) {
                throw std::runtime_error("JSON member 'materialx_materials[" + std::to_string(i) + "]' must be an object");
            }
            MaterialXMaterialConfig materialConfig;
            if (entry.HasMember("enabled")) {
                const auto& value = entry["enabled"];
                if (!value.IsBool()) {
                    throw std::runtime_error("JSON member 'materialx_materials[" + std::to_string(i) +
                                             "].enabled' must be a boolean");
                }
                materialConfig.enabled = value.GetBool();
            }
            if (entry.HasMember("document")) {
                const auto& value = entry["document"];
                if (!value.IsString()) {
                    throw std::runtime_error("JSON member 'materialx_materials[" + std::to_string(i) +
                                             "].document' must be a string");
                }
                materialConfig.documentPath = value.GetString();
            }
            if (entry.HasMember("shader_key")) {
                const auto& value = entry["shader_key"];
                if (!value.IsString()) {
                    throw std::runtime_error("JSON member 'materialx_materials[" + std::to_string(i) +
                                             "].shader_key' must be a string");
                }
                materialConfig.shaderKey = value.GetString();
            }
            if (entry.HasMember("material")) {
                const auto& value = entry["material"];
                if (!value.IsString()) {
                    throw std::runtime_error("JSON member 'materialx_materials[" + std::to_string(i) +
                                             "].material' must be a string");
                }
                materialConfig.materialName = value.GetString();
            }
            if (entry.HasMember("use_constant_color")) {
                const auto& value = entry["use_constant_color"];
                if (!value.IsBool()) {
                    throw std::runtime_error("JSON member 'materialx_materials[" + std::to_string(i) +
                                             "].use_constant_color' must be a boolean");
                }
                materialConfig.useConstantColor = value.GetBool();
            }
            if (entry.HasMember("constant_color")) {
                const auto& value = entry["constant_color"];
                if (!value.IsArray() || value.Size() != 3) {
                    throw std::runtime_error("JSON member 'materialx_materials[" + std::to_string(i) +
                                             "].constant_color' must be an array of 3 numbers");
                }
                for (rapidjson::SizeType channel = 0; channel < 3; ++channel) {
                    if (!value[channel].IsNumber()) {
                        throw std::runtime_error("JSON member 'materialx_materials[" + std::to_string(i) +
                                                 "].constant_color[" + std::to_string(channel) +
                                                 "]' must be a number");
                    }
                    materialConfig.constantColor[channel] = static_cast<float>(value[channel].GetDouble());
                }
            }

            if (materialConfig.shaderKey.empty()) {
                throw std::runtime_error("JSON member 'materialx_materials[" + std::to_string(i) +
                                         "].shader_key' must be provided");
            }
            if (materialConfig.documentPath.empty() && !materialConfig.useConstantColor) {
                throw std::runtime_error("JSON member 'materialx_materials[" + std::to_string(i) +
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

    if (document.HasMember("gui_font")) {
        const auto& fontValue = document["gui_font"];
        if (!fontValue.IsObject()) {
            throw std::runtime_error("JSON member 'gui_font' must be an object");
        }
        if (fontValue.HasMember("use_freetype")) {
            const auto& value = fontValue["use_freetype"];
            if (!value.IsBool()) {
                throw std::runtime_error("JSON member 'gui_font.use_freetype' must be a boolean");
            }
            config.guiFont.useFreeType = value.GetBool();
        }
        if (fontValue.HasMember("font_path")) {
            const auto& value = fontValue["font_path"];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member 'gui_font.font_path' must be a string");
            }
            config.guiFont.fontPath = value.GetString();
        }
        if (fontValue.HasMember("font_size")) {
            const auto& value = fontValue["font_size"];
            if (!value.IsNumber()) {
                throw std::runtime_error("JSON member 'gui_font.font_size' must be a number");
            }
            config.guiFont.fontSize = static_cast<float>(value.GetDouble());
        }
    }

    if (document.HasMember("gui_opacity")) {
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

    auto addStringMember = [&](const char* name, const std::string& value) {
        rapidjson::Value nameValue(name, allocator);
        rapidjson::Value stringValue(value.c_str(), allocator);
        document.AddMember(nameValue, stringValue, allocator);
    };

    document.AddMember("window_width", config.width, allocator);
    document.AddMember("window_height", config.height, allocator);
    addStringMember("lua_script", config.scriptPath.string());
    addStringMember("window_title", config.windowTitle);
    document.AddMember("lua_debug", config.luaDebug, allocator);

    std::filesystem::path scriptsDir = config.scriptPath.parent_path();
    if (!scriptsDir.empty()) {
        addStringMember("scripts_directory", scriptsDir.string());
    }

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

    rapidjson::Value bgfxObject(rapidjson::kObjectType);
    bgfxObject.AddMember("renderer",
                         rapidjson::Value(config.bgfx.renderer.c_str(), allocator),
                         allocator);
    document.AddMember("bgfx", bgfxObject, allocator);

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
    document.AddMember("materialx", materialObject, allocator);

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
        document.AddMember("materialx_materials", materialsArray, allocator);
    }

    rapidjson::Value fontObject(rapidjson::kObjectType);
    fontObject.AddMember("use_freetype", config.guiFont.useFreeType, allocator);
    fontObject.AddMember("font_path",
                         rapidjson::Value(config.guiFont.fontPath.string().c_str(), allocator),
                         allocator);
    fontObject.AddMember("font_size", config.guiFont.fontSize, allocator);
    document.AddMember("gui_font", fontObject, allocator);

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
    document.AddMember("input_bindings", bindingsObject, allocator);

    std::filesystem::path projectRoot = scriptsDir.parent_path();
    if (!projectRoot.empty()) {
        addStringMember("project_root", projectRoot.string());
        addStringMember("shaders_directory", (projectRoot / "shaders").string());
    } else {
        addStringMember("shaders_directory", "shaders");
    }

    document.AddMember("gui_opacity", config.guiOpacity, allocator);

    if (!configPath.empty()) {
        addStringMember("config_file", configPath.string());
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    return buffer.GetString();
}

}  // namespace sdl3cpp::services::impl
