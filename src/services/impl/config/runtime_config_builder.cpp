#include "runtime_config_builder.hpp"

#include "services/interfaces/config_types.hpp"

using sdl3cpp::services::InputBindings;
using sdl3cpp::services::RuntimeConfig;

#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>

namespace {
const rapidjson::Value* GetObjectMember(const rapidjson::Value& parent,
                                        const char* name,
                                        const char* fullName) {
    if (!parent.HasMember(name)) {
        return nullptr;
    }
    const auto& value = parent[name];
    if (!value.IsObject()) {
        throw std::runtime_error("JSON member '" + std::string(fullName) + "' must be an object");
    }
    return &value;
}

uint32_t ParseDimension(const rapidjson::Value& parent, const char* name, const std::string& path) {
    if (!parent.HasMember(name)) {
        return 0;
    }
    const auto& value = parent[name];
    if (value.IsUint()) {
        return value.GetUint();
    }
    if (value.IsInt()) {
        int maybeValue = value.GetInt();
        if (maybeValue >= 0) {
            return static_cast<uint32_t>(maybeValue);
        }
    }
    throw std::runtime_error("JSON member '" + path + "." + std::string(name) + "' must be a non-negative integer");
}

uint32_t ParseDimensionValue(const rapidjson::Value& value, const char* name) {
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
}

void ValidateArrayLength(const rapidjson::Value& value,
                         const std::string& path,
                         rapidjson::SizeType expected) {
    if (!value.IsArray() || value.Size() != expected) {
        throw std::runtime_error("JSON member '" + path + "' must be an array of " +
                                 std::to_string(expected) + " numbers");
    }
}
}  // namespace

namespace sdl3cpp::services::impl {

RuntimeConfigBuilder::RuntimeConfigBuilder(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

RuntimeConfig RuntimeConfigBuilder::Build(const rapidjson::Document& document,
                                          const std::filesystem::path& configPath) const {
    RuntimeConfig config;
    auto pathsValue = GetObjectMember(document, "paths", "paths");
    auto windowValue = GetObjectMember(document, "window", "window");
    const auto* windowSizeValue = windowValue ? GetObjectMember(*windowValue, "size", "window.size") : nullptr;
    auto inputValue = GetObjectMember(document, "input", "input");
    const auto* inputBindingsValue = inputValue ? GetObjectMember(*inputValue, "bindings", "input.bindings") : nullptr;
    auto renderingValue = GetObjectMember(document, "rendering", "rendering");
    auto guiValue = GetObjectMember(document, "gui", "gui");

    std::optional<std::filesystem::path> projectRoot;
    if (pathsValue && pathsValue->HasMember("project_root")) {
        const auto& value = (*pathsValue)["project_root"];
        if (!value.IsString()) {
            throw std::runtime_error("JSON member 'paths.project_root' must be a string");
        }
        std::filesystem::path candidate(value.GetString());
        projectRoot = candidate.is_absolute()
            ? std::filesystem::weakly_canonical(candidate)
            : std::filesystem::weakly_canonical(configPath.parent_path() / candidate);
    } else if (document.HasMember("project_root")) {
        const auto& value = document["project_root"];
        if (!value.IsString()) {
            throw std::runtime_error("JSON member 'project_root' must be a string");
        }
        std::filesystem::path candidate(value.GetString());
        projectRoot = candidate.is_absolute()
            ? std::filesystem::weakly_canonical(candidate)
            : std::filesystem::weakly_canonical(configPath.parent_path() / candidate);
    }
    std::filesystem::path baseRoot = configPath.empty()
        ? std::filesystem::current_path()
        : configPath.parent_path();
    std::filesystem::path resolvedRoot = projectRoot ? *projectRoot : baseRoot;
    std::error_code rootError;
    resolvedRoot = std::filesystem::weakly_canonical(resolvedRoot, rootError);
    if (rootError) {
        throw std::runtime_error("Failed to resolve project root: " + rootError.message());
    }
    config.projectRoot = resolvedRoot;

    if (windowSizeValue) {
        if (windowSizeValue->HasMember("width")) {
            config.width = ParseDimensionValue((*windowSizeValue)["width"], "window.size.width");
        }
        if (windowSizeValue->HasMember("height")) {
            config.height = ParseDimensionValue((*windowSizeValue)["height"], "window.size.height");
        }
    } else {
        if (document.HasMember("window_width")) {
            config.width = ParseDimension(document, "window_width", "window_width");
        }
        if (document.HasMember("window_height")) {
            config.height = ParseDimension(document, "window_height", "window_height");
        }
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
        atmosphericsValue = GetObjectMember(*renderingValue, "atmospherics", "rendering.atmospherics");
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
            ValidateArrayLength(value, atmosphericsPath + "." + name, 3);
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
        bgfxValue = GetObjectMember(*renderingValue, "bgfx", "rendering.bgfx");
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
    if (bgfxValue && bgfxValue->HasMember("renderer")) {
        const auto& value = (*bgfxValue)["renderer"];
        if (!value.IsString()) {
            throw std::runtime_error("JSON member '" + bgfxPath + ".renderer' must be a string");
        }
        config.bgfx.renderer = value.GetString();
    }

    const rapidjson::Value* materialValue = nullptr;
    std::string materialPath = "materialx";
    if (renderingValue) {
        materialValue = GetObjectMember(*renderingValue, "materialx", "rendering.materialx");
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
            ValidateArrayLength(value, materialPath + ".constant_color", 3);
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
                ValidateArrayLength(value, materialsPath + "[" + std::to_string(i) + "].constant_color", 3);
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
        if (logger_) {
            logger_->Trace("RuntimeConfigBuilder", "Build",
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

    const auto* budgetsValue = GetObjectMember(document, "budgets", "budgets");
    if (budgetsValue) {
        readSizeT(*budgetsValue, "vram_mb", "budgets", config.budgets.vramMB);
        readUint32(*budgetsValue, "max_texture_dim", "budgets", config.budgets.maxTextureDim);
        readSizeT(*budgetsValue, "gui_text_cache_entries", "budgets", config.budgets.guiTextCacheEntries);
        readSizeT(*budgetsValue, "gui_svg_cache_entries", "budgets", config.budgets.guiSvgCacheEntries);
    }

    const auto* crashRecoveryValue = GetObjectMember(document, "crash_recovery", "crash_recovery");
    if (crashRecoveryValue) {
        readUint32(*crashRecoveryValue, "heartbeat_timeout_ms",
                   "crash_recovery", config.crashRecovery.heartbeatTimeoutMs);
        readUint32(*crashRecoveryValue, "heartbeat_poll_interval_ms",
                   "crash_recovery", config.crashRecovery.heartbeatPollIntervalMs);
        readSizeT(*crashRecoveryValue, "memory_limit_mb",
                  "crash_recovery", config.crashRecovery.memoryLimitMB);
        readSizeT(*crashRecoveryValue, "max_consecutive_gpu_timeouts",
                  "crash_recovery", config.crashRecovery.maxConsecutiveGpuTimeouts);
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

    const auto* validationValue = GetObjectMember(document, "validation_tour", "validation_tour");
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
                ValidateArrayLength(value, path, 3);
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
                readFloat3(cameraValue["position"], basePath + ".camera.position", checkpoint.camera.position);
                if (!cameraValue.HasMember("look_at")) {
                    throw std::runtime_error("JSON member '" + basePath + ".camera.look_at' is required");
                }
                readFloat3(cameraValue["look_at"], basePath + ".camera.look_at", checkpoint.camera.lookAt);
                if (cameraValue.HasMember("up")) {
                    readFloat3(cameraValue["up"], basePath + ".camera.up", checkpoint.camera.up);
                }
                if (cameraValue.HasMember("fov_degrees")) {
                    const auto& value = cameraValue["fov_degrees"];
                    if (!value.IsNumber()) {
                        throw std::runtime_error("JSON member '" + basePath + ".camera.fov_degrees' must be a number");
                    }
                    checkpoint.camera.fovDegrees = static_cast<float>(value.GetDouble());
                }
                if (cameraValue.HasMember("near")) {
                    const auto& value = cameraValue["near"];
                    if (!value.IsNumber()) {
                        throw std::runtime_error("JSON member '" + basePath + ".camera.near' must be a number");
                    }
                    checkpoint.camera.nearPlane = static_cast<float>(value.GetDouble());
                }
                if (cameraValue.HasMember("far")) {
                    const auto& value = cameraValue["far"];
                    if (!value.IsNumber()) {
                        throw std::runtime_error("JSON member '" + basePath + ".camera.far' must be a number");
                    }
                    checkpoint.camera.farPlane = static_cast<float>(value.GetDouble());
                }

                auto readFloatInRange = [&](const rapidjson::Value& value,
                                            const std::string& path,
                                            double minValue,
                                            double maxValue) -> float {
                    if (!value.IsNumber()) {
                        throw std::runtime_error("JSON member '" + path + "' must be a number");
                    }
                    const double rawValue = value.GetDouble();
                    if (rawValue < minValue || rawValue > maxValue) {
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
                        ValidateArrayLength(value, path, 3);
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

}  // namespace sdl3cpp::services::impl
