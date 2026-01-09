#include "json_config_service.hpp"
#include "json_config_document_loader.hpp"
#include "json_config_migration_service.hpp"
#include "json_config_schema_validator.hpp"
#include "json_config_schema_version.hpp"
#include "json_config_version_validator.hpp"
#include "../interfaces/i_logger.hpp"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <array>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace sdl3cpp::services::impl {

namespace {
const char* SceneSourceName(SceneSource source) {
    switch (source) {
        case SceneSource::Config:
            return "config";
        case SceneSource::Lua:
            return "lua";
        default:
            return "config";
    }
}

SceneSource ParseSceneSource(const std::string& value, const std::string& jsonPath) {
    if (value == "config") {
        return SceneSource::Config;
    }
    if (value == "lua") {
        return SceneSource::Lua;
    }
    throw std::runtime_error("JSON member '" + jsonPath + "' must be 'config' or 'lua'");
}
}  // namespace

JsonConfigService::JsonConfigService(std::shared_ptr<ILogger> logger,
                                     const char* argv0,
                                     std::shared_ptr<IProbeService> probeService)
    : logger_(std::move(logger)),
      probeService_(std::move(probeService)),
      configJson_(),
      config_(RuntimeConfig{}) {
    if (logger_) {
        logger_->Trace("JsonConfigService", "JsonConfigService",
                       "argv0=" + std::string(argv0 ? argv0 : ""));
    }
    config_.scriptPath = FindScriptPath(argv0);
    configJson_ = BuildConfigJson(config_, {});
    logger_->Info("JsonConfigService initialized with default configuration");
}

JsonConfigService::JsonConfigService(std::shared_ptr<ILogger> logger,
                                     const std::filesystem::path& configPath,
                                     bool dumpConfig,
                                     std::shared_ptr<IProbeService> probeService)
    : logger_(std::move(logger)),
      probeService_(std::move(probeService)),
      configJson_(),
      config_(LoadFromJson(logger_, probeService_, configPath, dumpConfig, &configJson_)) {
    if (logger_) {
        logger_->Trace("JsonConfigService", "JsonConfigService",
                       "configPath=" + configPath.string() +
                       ", dumpConfig=" + std::string(dumpConfig ? "true" : "false"));
    }
    logger_->Info("JsonConfigService initialized from config file: " + configPath.string());
}

JsonConfigService::JsonConfigService(std::shared_ptr<ILogger> logger,
                                     const RuntimeConfig& config,
                                     std::shared_ptr<IProbeService> probeService)
    : logger_(std::move(logger)),
      probeService_(std::move(probeService)),
      configJson_(BuildConfigJson(config, {})),
      config_(config) {
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
                                              std::shared_ptr<IProbeService> probeService,
                                              const std::filesystem::path& configPath,
                                              bool dumpConfig,
                                              std::string* configJson) {
    std::string args = "configPath=" + configPath.string() +
        ", dumpConfig=" + (dumpConfig ? "true" : "false");
    logger->Trace("JsonConfigService", "LoadFromJson", args);

    json_config::JsonConfigDocumentLoader documentLoader(logger);
    rapidjson::Document document = documentLoader.Load(configPath);

    json_config::JsonConfigVersionValidator versionValidator(logger);
    const auto activeVersion = versionValidator.Validate(document, configPath);
    if (activeVersion && *activeVersion != json_config::kRuntimeConfigSchemaVersion) {
        json_config::JsonConfigMigrationService migrationService(logger, probeService);
        const bool migrated = migrationService.Apply(document,
                                                     *activeVersion,
                                                     json_config::kRuntimeConfigSchemaVersion,
                                                     configPath);
        if (!migrated) {
            throw std::runtime_error("Unsupported schema version " + std::to_string(*activeVersion) +
                                     " in " + configPath.string() +
                                     "; expected " + std::to_string(json_config::kRuntimeConfigSchemaVersion) +
                                     " (see config/schema/MIGRATIONS.md)");
        }
    }

    json_config::JsonConfigSchemaValidator schemaValidator(logger, probeService);
    schemaValidator.ValidateOrThrow(document, configPath);

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
    const auto* runtimeValue = getObjectMember(document, "runtime", "runtime");
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

    if (runtimeValue && runtimeValue->HasMember("scene_source")) {
        const auto& value = (*runtimeValue)["scene_source"];
        if (!value.IsString()) {
            throw std::runtime_error("JSON member 'runtime.scene_source' must be a string");
        }
        config.sceneSource = ParseSceneSource(value.GetString(), "runtime.scene_source");
    }

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

    auto readSizeT = [](const rapidjson::Value& parent,
                        const char* name,
                        const std::string& path,
                        size_t& target) {
        if (!parent.HasMember(name)) {
            return;
        }
        const auto& value = parent[name];
        if (!value.IsNumber()) {
            throw std::runtime_error("JSON member '" + path + "." + std::string(name) + "' must be a number");
        }
        const double rawValue = value.GetDouble();
        if (rawValue < 0.0) {
            throw std::runtime_error("JSON member '" + path + "." + std::string(name) + "' must be non-negative");
        }
        target = static_cast<size_t>(rawValue);
    };

    auto readUint32 = [](const rapidjson::Value& parent,
                         const char* name,
                         const std::string& path,
                         uint32_t& target) {
        if (!parent.HasMember(name)) {
            return;
        }
        const auto& value = parent[name];
        if (!value.IsNumber()) {
            throw std::runtime_error("JSON member '" + path + "." + std::string(name) + "' must be a number");
        }
        const double rawValue = value.GetDouble();
        if (rawValue < 0.0) {
            throw std::runtime_error("JSON member '" + path + "." + std::string(name) + "' must be non-negative");
        }
        target = static_cast<uint32_t>(rawValue);
    };

    const auto* budgetsValue = getObjectMember(document, "budgets", "budgets");
    if (budgetsValue) {
        readSizeT(*budgetsValue, "vram_mb", "budgets", config.budgets.vramMB);
        readUint32(*budgetsValue, "max_texture_dim", "budgets", config.budgets.maxTextureDim);
        readSizeT(*budgetsValue, "gui_text_cache_entries", "budgets", config.budgets.guiTextCacheEntries);
        readSizeT(*budgetsValue, "gui_svg_cache_entries", "budgets", config.budgets.guiSvgCacheEntries);
    }

    const auto* crashRecoveryValue = getObjectMember(document, "crash_recovery", "crash_recovery");
    if (crashRecoveryValue) {
        readUint32(*crashRecoveryValue, "heartbeat_timeout_ms",
                   "crash_recovery", config.crashRecovery.heartbeatTimeoutMs);
        readUint32(*crashRecoveryValue, "heartbeat_poll_interval_ms",
                   "crash_recovery", config.crashRecovery.heartbeatPollIntervalMs);
        readSizeT(*crashRecoveryValue, "memory_limit_mb",
                  "crash_recovery", config.crashRecovery.memoryLimitMB);
        readSizeT(*crashRecoveryValue, "max_consecutive_gpu_timeouts",
                  "crash_recovery", config.crashRecovery.maxConsecutiveGpuTimeouts);
        readSizeT(*crashRecoveryValue, "max_lua_failures",
                  "crash_recovery", config.crashRecovery.maxLuaFailures);
        readSizeT(*crashRecoveryValue, "max_file_format_errors",
                  "crash_recovery", config.crashRecovery.maxFileFormatErrors);
        readSizeT(*crashRecoveryValue, "max_memory_warnings",
                  "crash_recovery", config.crashRecovery.maxMemoryWarnings);
        if (crashRecoveryValue->HasMember("gpu_hang_frame_time_multiplier")) {
            const auto& value = (*crashRecoveryValue)["gpu_hang_frame_time_multiplier"];
            if (!value.IsNumber()) {
                throw std::runtime_error("JSON member 'crash_recovery.gpu_hang_frame_time_multiplier' must be a number");
            }
            const double rawValue = value.GetDouble();
            if (rawValue < 0.0) {
                throw std::runtime_error("JSON member 'crash_recovery.gpu_hang_frame_time_multiplier' must be non-negative");
            }
            config.crashRecovery.gpuHangFrameTimeMultiplier = rawValue;
        }
    }

    const auto* validationValue = getObjectMember(document, "validation_tour", "validation_tour");
    if (validationValue) {
        auto readBool = [&](const char* name, bool& target) {
            if (!validationValue->HasMember(name)) {
                return;
            }
            const auto& value = (*validationValue)[name];
            if (!value.IsBool()) {
                throw std::runtime_error("JSON member 'validation_tour." + std::string(name) + "' must be a boolean");
            }
            target = value.GetBool();
        };
        readBool("enabled", config.validationTour.enabled);
        readBool("fail_on_mismatch", config.validationTour.failOnMismatch);
        readUint32(*validationValue, "warmup_frames", "validation_tour", config.validationTour.warmupFrames);
        readUint32(*validationValue, "capture_frames", "validation_tour", config.validationTour.captureFrames);

        if (validationValue->HasMember("output_dir")) {
            const auto& value = (*validationValue)["output_dir"];
            if (!value.IsString()) {
                throw std::runtime_error("JSON member 'validation_tour.output_dir' must be a string");
            }
            config.validationTour.outputDir = value.GetString();
        }

        if (validationValue->HasMember("checkpoints")) {
            const auto& checkpointsValue = (*validationValue)["checkpoints"];
            if (!checkpointsValue.IsArray()) {
                throw std::runtime_error("JSON member 'validation_tour.checkpoints' must be an array");
            }
            config.validationTour.checkpoints.clear();
            config.validationTour.checkpoints.reserve(checkpointsValue.Size());

            auto readFloat3 = [&](const rapidjson::Value& value,
                                  const std::string& path,
                                  std::array<float, 3>& target) {
                if (!value.IsArray() || value.Size() != 3) {
                    throw std::runtime_error("JSON member '" + path + "' must be an array of 3 numbers");
                }
                for (rapidjson::SizeType i = 0; i < 3; ++i) {
                    if (!value[i].IsNumber()) {
                        throw std::runtime_error("JSON member '" + path + "[" + std::to_string(i) +
                                                 "]' must be a number");
                    }
                    target[i] = static_cast<float>(value[i].GetDouble());
                }
            };

            for (rapidjson::SizeType i = 0; i < checkpointsValue.Size(); ++i) {
                const auto& entry = checkpointsValue[i];
                const std::string basePath = "validation_tour.checkpoints[" + std::to_string(i) + "]";
                if (!entry.IsObject()) {
                    throw std::runtime_error("JSON member '" + basePath + "' must be an object");
                }
                ValidationCheckpointConfig checkpoint;

                if (!entry.HasMember("id") || !entry["id"].IsString()) {
                    throw std::runtime_error("JSON member '" + basePath + ".id' must be a string");
                }
                checkpoint.id = entry["id"].GetString();

                if (!entry.HasMember("camera") || !entry["camera"].IsObject()) {
                    throw std::runtime_error("JSON member '" + basePath + ".camera' must be an object");
                }
                const auto& cameraValue = entry["camera"];
                if (!cameraValue.HasMember("position")) {
                    throw std::runtime_error("JSON member '" + basePath + ".camera.position' is required");
                }
                readFloat3(cameraValue["position"], basePath + ".camera.position",
                           checkpoint.camera.position);
                if (!cameraValue.HasMember("look_at")) {
                    throw std::runtime_error("JSON member '" + basePath + ".camera.look_at' is required");
                }
                readFloat3(cameraValue["look_at"], basePath + ".camera.look_at",
                           checkpoint.camera.lookAt);
                if (cameraValue.HasMember("up")) {
                    readFloat3(cameraValue["up"], basePath + ".camera.up",
                               checkpoint.camera.up);
                }
                if (cameraValue.HasMember("fov_degrees")) {
                    const auto& value = cameraValue["fov_degrees"];
                    if (!value.IsNumber()) {
                        throw std::runtime_error("JSON member '" + basePath +
                                                 ".camera.fov_degrees' must be a number");
                    }
                    checkpoint.camera.fovDegrees = static_cast<float>(value.GetDouble());
                }
                if (cameraValue.HasMember("near")) {
                    const auto& value = cameraValue["near"];
                    if (!value.IsNumber()) {
                        throw std::runtime_error("JSON member '" + basePath +
                                                 ".camera.near' must be a number");
                    }
                    checkpoint.camera.nearPlane = static_cast<float>(value.GetDouble());
                }
                if (cameraValue.HasMember("far")) {
                    const auto& value = cameraValue["far"];
                    if (!value.IsNumber()) {
                        throw std::runtime_error("JSON member '" + basePath +
                                                 ".camera.far' must be a number");
                    }
                    checkpoint.camera.farPlane = static_cast<float>(value.GetDouble());
                }

                auto readFloatInRange = [&](const rapidjson::Value& value,
                                            const std::string& path,
                                            float minValue,
                                            float maxValue) -> float {
                    if (!value.IsNumber()) {
                        throw std::runtime_error("JSON member '" + path + "' must be a number");
                    }
                    const double rawValue = value.GetDouble();
                    const double minAllowed = static_cast<double>(minValue);
                    const double maxAllowed = static_cast<double>(maxValue);
                    if (rawValue < minAllowed || rawValue > maxAllowed) {
                        throw std::runtime_error("JSON member '" + path + "' must be between " +
                                                 std::to_string(minValue) + " and " +
                                                 std::to_string(maxValue));
                    }
                    return static_cast<float>(rawValue);
                };

                if (entry.HasMember("expected")) {
                    if (!entry["expected"].IsObject()) {
                        throw std::runtime_error("JSON member '" + basePath + ".expected' must be an object");
                    }
                    const auto& expectedValue = entry["expected"];
                    if (!expectedValue.HasMember("image") || !expectedValue["image"].IsString()) {
                        throw std::runtime_error("JSON member '" + basePath + ".expected.image' must be a string");
                    }
                    checkpoint.expected.enabled = true;
                    checkpoint.expected.imagePath = expectedValue["image"].GetString();
                    if (expectedValue.HasMember("tolerance")) {
                        checkpoint.expected.tolerance = readFloatInRange(
                            expectedValue["tolerance"],
                            basePath + ".expected.tolerance",
                            0.0f,
                            1.0f);
                    }
                    if (expectedValue.HasMember("max_diff_pixels")) {
                        const auto& value = expectedValue["max_diff_pixels"];
                        if (!value.IsNumber()) {
                            throw std::runtime_error("JSON member '" + basePath +
                                                     ".expected.max_diff_pixels' must be a number");
                        }
                        const double rawValue = value.GetDouble();
                        if (rawValue < 0.0) {
                            throw std::runtime_error("JSON member '" + basePath +
                                                     ".expected.max_diff_pixels' must be non-negative");
                        }
                        checkpoint.expected.maxDiffPixels = static_cast<size_t>(rawValue);
                    }
                }

                if (entry.HasMember("checks")) {
                    const auto& checksValue = entry["checks"];
                    if (!checksValue.IsArray()) {
                        throw std::runtime_error("JSON member '" + basePath + ".checks' must be an array");
                    }
                    checkpoint.checks.clear();
                    checkpoint.checks.reserve(checksValue.Size());

                    auto readFloat3Field = [&](const rapidjson::Value& value,
                                               const std::string& path,
                                               std::array<float, 3>& target) {
                        if (!value.IsArray() || value.Size() != 3) {
                            throw std::runtime_error("JSON member '" + path + "' must be an array of 3 numbers");
                        }
                        for (rapidjson::SizeType index = 0; index < 3; ++index) {
                            if (!value[index].IsNumber()) {
                                throw std::runtime_error("JSON member '" + path + "[" + std::to_string(index) +
                                                         "]' must be a number");
                            }
                            target[index] = static_cast<float>(value[index].GetDouble());
                        }
                    };

                    for (rapidjson::SizeType checkIndex = 0; checkIndex < checksValue.Size(); ++checkIndex) {
                        const auto& checkValue = checksValue[checkIndex];
                        const std::string checkPath = basePath + ".checks[" + std::to_string(checkIndex) + "]";
                        if (!checkValue.IsObject()) {
                            throw std::runtime_error("JSON member '" + checkPath + "' must be an object");
                        }
                        if (!checkValue.HasMember("type") || !checkValue["type"].IsString()) {
                            throw std::runtime_error("JSON member '" + checkPath + ".type' must be a string");
                        }
                        ValidationCheckConfig check{};
                        check.type = checkValue["type"].GetString();

                        if (check.type == "non_black_ratio") {
                            if (checkValue.HasMember("threshold")) {
                                check.threshold = readFloatInRange(
                                    checkValue["threshold"],
                                    checkPath + ".threshold",
                                    0.0f,
                                    1.0f);
                            }
                            if (checkValue.HasMember("min_ratio")) {
                                check.minValue = readFloatInRange(
                                    checkValue["min_ratio"],
                                    checkPath + ".min_ratio",
                                    0.0f,
                                    1.0f);
                            }
                            if (checkValue.HasMember("max_ratio")) {
                                check.maxValue = readFloatInRange(
                                    checkValue["max_ratio"],
                                    checkPath + ".max_ratio",
                                    0.0f,
                                    1.0f);
                            }
                            if (check.minValue > check.maxValue) {
                                throw std::runtime_error("JSON member '" + checkPath +
                                                         "' must have min_ratio <= max_ratio");
                            }
                        } else if (check.type == "luma_range") {
                            if (!checkValue.HasMember("min_luma") || !checkValue.HasMember("max_luma")) {
                                throw std::runtime_error("JSON member '" + checkPath +
                                                         "' must include min_luma and max_luma");
                            }
                            check.minValue = readFloatInRange(
                                checkValue["min_luma"],
                                checkPath + ".min_luma",
                                0.0f,
                                1.0f);
                            check.maxValue = readFloatInRange(
                                checkValue["max_luma"],
                                checkPath + ".max_luma",
                                0.0f,
                                1.0f);
                            if (check.minValue > check.maxValue) {
                                throw std::runtime_error("JSON member '" + checkPath +
                                                         "' must have min_luma <= max_luma");
                            }
                        } else if (check.type == "mean_color") {
                            if (!checkValue.HasMember("color")) {
                                throw std::runtime_error("JSON member '" + checkPath + ".color' is required");
                            }
                            readFloat3Field(checkValue["color"], checkPath + ".color", check.color);
                            if (checkValue.HasMember("tolerance")) {
                                check.tolerance = readFloatInRange(
                                    checkValue["tolerance"],
                                    checkPath + ".tolerance",
                                    0.0f,
                                    1.0f);
                            }
                        } else if (check.type == "sample_points") {
                            if (!checkValue.HasMember("points") || !checkValue["points"].IsArray()) {
                                throw std::runtime_error("JSON member '" + checkPath + ".points' must be an array");
                            }
                            const auto& pointsValue = checkValue["points"];
                            check.points.clear();
                            check.points.reserve(pointsValue.Size());
                            for (rapidjson::SizeType pointIndex = 0; pointIndex < pointsValue.Size(); ++pointIndex) {
                                const auto& pointValue = pointsValue[pointIndex];
                                const std::string pointPath = checkPath + ".points[" + std::to_string(pointIndex) + "]";
                                if (!pointValue.IsObject()) {
                                    throw std::runtime_error("JSON member '" + pointPath + "' must be an object");
                                }
                                ValidationSamplePointConfig point{};
                                if (!pointValue.HasMember("x") || !pointValue.HasMember("y")) {
                                    throw std::runtime_error("JSON member '" + pointPath +
                                                             "' must include x and y");
                                }
                                point.x = readFloatInRange(pointValue["x"], pointPath + ".x", 0.0f, 1.0f);
                                point.y = readFloatInRange(pointValue["y"], pointPath + ".y", 0.0f, 1.0f);
                                if (!pointValue.HasMember("color")) {
                                    throw std::runtime_error("JSON member '" + pointPath + ".color' is required");
                                }
                                readFloat3Field(pointValue["color"], pointPath + ".color", point.color);
                                if (pointValue.HasMember("tolerance")) {
                                    point.tolerance = readFloatInRange(
                                        pointValue["tolerance"],
                                        pointPath + ".tolerance",
                                        0.0f,
                                        1.0f);
                                }
                                check.points.push_back(std::move(point));
                            }
                        } else {
                            throw std::runtime_error("JSON member '" + checkPath + ".type' is unsupported");
                        }

                        checkpoint.checks.push_back(std::move(check));
                    }
                }

                if (!checkpoint.expected.enabled && checkpoint.checks.empty()) {
                    throw std::runtime_error("JSON member '" + basePath +
                                             "' must define 'expected' or 'checks'");
                }

                config.validationTour.checkpoints.push_back(std::move(checkpoint));
            }
        }
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

    document.AddMember("schema_version", json_config::kRuntimeConfigSchemaVersion, allocator);
    document.AddMember("configVersion", json_config::kRuntimeConfigSchemaVersion, allocator);

    rapidjson::Value scriptsObject(rapidjson::kObjectType);
    addStringMember(scriptsObject, "entry", config.scriptPath.string());
    scriptsObject.AddMember("lua_debug", config.luaDebug, allocator);
    document.AddMember("scripts", scriptsObject, allocator);

    rapidjson::Value runtimeObject(rapidjson::kObjectType);
    addStringMember(runtimeObject, "scene_source", SceneSourceName(config.sceneSource));
    document.AddMember("runtime", runtimeObject, allocator);

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

    rapidjson::Value budgetsObject(rapidjson::kObjectType);
    budgetsObject.AddMember("vram_mb", static_cast<uint64_t>(config.budgets.vramMB), allocator);
    budgetsObject.AddMember("max_texture_dim", config.budgets.maxTextureDim, allocator);
    budgetsObject.AddMember("gui_text_cache_entries",
                            static_cast<uint64_t>(config.budgets.guiTextCacheEntries),
                            allocator);
    budgetsObject.AddMember("gui_svg_cache_entries",
                            static_cast<uint64_t>(config.budgets.guiSvgCacheEntries),
                            allocator);
    document.AddMember("budgets", budgetsObject, allocator);

    rapidjson::Value crashObject(rapidjson::kObjectType);
    crashObject.AddMember("heartbeat_timeout_ms", config.crashRecovery.heartbeatTimeoutMs, allocator);
    crashObject.AddMember("heartbeat_poll_interval_ms", config.crashRecovery.heartbeatPollIntervalMs, allocator);
    crashObject.AddMember("memory_limit_mb", static_cast<uint64_t>(config.crashRecovery.memoryLimitMB), allocator);
    crashObject.AddMember("gpu_hang_frame_time_multiplier",
                          config.crashRecovery.gpuHangFrameTimeMultiplier, allocator);
    crashObject.AddMember("max_consecutive_gpu_timeouts",
                          static_cast<uint64_t>(config.crashRecovery.maxConsecutiveGpuTimeouts), allocator);
    crashObject.AddMember("max_lua_failures",
                          static_cast<uint64_t>(config.crashRecovery.maxLuaFailures), allocator);
    crashObject.AddMember("max_file_format_errors",
                          static_cast<uint64_t>(config.crashRecovery.maxFileFormatErrors), allocator);
    crashObject.AddMember("max_memory_warnings",
                          static_cast<uint64_t>(config.crashRecovery.maxMemoryWarnings), allocator);
    document.AddMember("crash_recovery", crashObject, allocator);

    rapidjson::Value validationObject(rapidjson::kObjectType);
    validationObject.AddMember("enabled", config.validationTour.enabled, allocator);
    validationObject.AddMember("fail_on_mismatch", config.validationTour.failOnMismatch, allocator);
    validationObject.AddMember("warmup_frames", config.validationTour.warmupFrames, allocator);
    validationObject.AddMember("capture_frames", config.validationTour.captureFrames, allocator);
    addStringMember(validationObject, "output_dir", config.validationTour.outputDir.string());

    if (!config.validationTour.checkpoints.empty()) {
        rapidjson::Value checkpointsArray(rapidjson::kArrayType);
        for (const auto& checkpoint : config.validationTour.checkpoints) {
            rapidjson::Value entry(rapidjson::kObjectType);
            entry.AddMember("id", rapidjson::Value(checkpoint.id.c_str(), allocator), allocator);

            rapidjson::Value cameraObject(rapidjson::kObjectType);
            rapidjson::Value cameraPosition(rapidjson::kArrayType);
            cameraPosition.PushBack(checkpoint.camera.position[0], allocator);
            cameraPosition.PushBack(checkpoint.camera.position[1], allocator);
            cameraPosition.PushBack(checkpoint.camera.position[2], allocator);
            cameraObject.AddMember("position", cameraPosition, allocator);

            rapidjson::Value cameraLookAt(rapidjson::kArrayType);
            cameraLookAt.PushBack(checkpoint.camera.lookAt[0], allocator);
            cameraLookAt.PushBack(checkpoint.camera.lookAt[1], allocator);
            cameraLookAt.PushBack(checkpoint.camera.lookAt[2], allocator);
            cameraObject.AddMember("look_at", cameraLookAt, allocator);

            rapidjson::Value cameraUp(rapidjson::kArrayType);
            cameraUp.PushBack(checkpoint.camera.up[0], allocator);
            cameraUp.PushBack(checkpoint.camera.up[1], allocator);
            cameraUp.PushBack(checkpoint.camera.up[2], allocator);
            cameraObject.AddMember("up", cameraUp, allocator);

            cameraObject.AddMember("fov_degrees", checkpoint.camera.fovDegrees, allocator);
            cameraObject.AddMember("near", checkpoint.camera.nearPlane, allocator);
            cameraObject.AddMember("far", checkpoint.camera.farPlane, allocator);
            entry.AddMember("camera", cameraObject, allocator);

            if (checkpoint.expected.enabled) {
                rapidjson::Value expectedObject(rapidjson::kObjectType);
                expectedObject.AddMember("image",
                                         rapidjson::Value(checkpoint.expected.imagePath.string().c_str(), allocator),
                                         allocator);
                expectedObject.AddMember("tolerance", checkpoint.expected.tolerance, allocator);
                expectedObject.AddMember("max_diff_pixels",
                                         static_cast<uint64_t>(checkpoint.expected.maxDiffPixels),
                                         allocator);
                entry.AddMember("expected", expectedObject, allocator);
            }

            if (!checkpoint.checks.empty()) {
                rapidjson::Value checksArray(rapidjson::kArrayType);
                for (const auto& check : checkpoint.checks) {
                    rapidjson::Value checkObject(rapidjson::kObjectType);
                    checkObject.AddMember("type", rapidjson::Value(check.type.c_str(), allocator), allocator);
                    if (check.type == "non_black_ratio") {
                        checkObject.AddMember("threshold", check.threshold, allocator);
                        checkObject.AddMember("min_ratio", check.minValue, allocator);
                        checkObject.AddMember("max_ratio", check.maxValue, allocator);
                    } else if (check.type == "luma_range") {
                        checkObject.AddMember("min_luma", check.minValue, allocator);
                        checkObject.AddMember("max_luma", check.maxValue, allocator);
                    } else if (check.type == "mean_color") {
                        rapidjson::Value colorValue(rapidjson::kArrayType);
                        colorValue.PushBack(check.color[0], allocator);
                        colorValue.PushBack(check.color[1], allocator);
                        colorValue.PushBack(check.color[2], allocator);
                        checkObject.AddMember("color", colorValue, allocator);
                        checkObject.AddMember("tolerance", check.tolerance, allocator);
                    } else if (check.type == "sample_points") {
                        rapidjson::Value pointsArray(rapidjson::kArrayType);
                        for (const auto& point : check.points) {
                            rapidjson::Value pointValue(rapidjson::kObjectType);
                            pointValue.AddMember("x", point.x, allocator);
                            pointValue.AddMember("y", point.y, allocator);
                            rapidjson::Value pointColor(rapidjson::kArrayType);
                            pointColor.PushBack(point.color[0], allocator);
                            pointColor.PushBack(point.color[1], allocator);
                            pointColor.PushBack(point.color[2], allocator);
                            pointValue.AddMember("color", pointColor, allocator);
                            pointValue.AddMember("tolerance", point.tolerance, allocator);
                            pointsArray.PushBack(pointValue, allocator);
                        }
                        checkObject.AddMember("points", pointsArray, allocator);
                    }
                    checksArray.PushBack(checkObject, allocator);
                }
                entry.AddMember("checks", checksArray, allocator);
            }

            checkpointsArray.PushBack(entry, allocator);
        }
        validationObject.AddMember("checkpoints", checkpointsArray, allocator);
    }
    document.AddMember("validation_tour", validationObject, allocator);

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
