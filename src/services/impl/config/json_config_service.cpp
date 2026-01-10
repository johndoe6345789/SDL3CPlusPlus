#include "json_config_service.hpp"
#include "json_config_schema_version.hpp"
#include "../workflow/workflow_config_pipeline.hpp"
#include "services/interfaces/i_logger.hpp"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <array>
#include <iostream>
#include <stdexcept>

namespace sdl3cpp::services::impl {

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
    config_.projectRoot = ResolveProjectRoot(argv0);
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
                       ", config.projectRoot=" + config.projectRoot.string() +
                       ", config.windowTitle=" + config.windowTitle);
    }
    logger_->Info("JsonConfigService initialized with explicit configuration");
}

std::filesystem::path JsonConfigService::ResolveProjectRoot(const char* argv0) {
    if (logger_) {
        logger_->Trace("JsonConfigService", "ResolveProjectRoot",
                       "argv0=" + std::string(argv0 ? argv0 : ""));
    }
    std::filesystem::path basePath;
    if (argv0 && *argv0 != '\0') {
        basePath = std::filesystem::path(argv0);
        if (basePath.is_relative()) {
            basePath = std::filesystem::current_path() / basePath;
        }
    } else {
        basePath = std::filesystem::current_path();
    }
    std::filesystem::path resolved = std::filesystem::weakly_canonical(basePath);
    if (std::filesystem::is_regular_file(resolved)) {
        resolved = resolved.parent_path();
    }
    return resolved;
}

RuntimeConfig JsonConfigService::LoadFromJson(std::shared_ptr<ILogger> logger,
                                              std::shared_ptr<IProbeService> probeService,
                                              const std::filesystem::path& configPath,
                                              bool dumpConfig,
                                              std::string* configJson) {
    std::string args = "configPath=" + configPath.string() +
        ", dumpConfig=" + (dumpConfig ? "true" : "false");
    logger->Trace("JsonConfigService", "LoadFromJson", args);

    WorkflowConfigPipeline pipeline(logger, probeService);
    WorkflowResult result = pipeline.Execute(configPath, nullptr);
    if (!result.document) {
        throw std::runtime_error("JsonConfigService::LoadFromJson: workflow pipeline returned null document");
    }

    const rapidjson::Document& document = *result.document;
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

    const auto* runtimeConfig = result.context.TryGet<RuntimeConfig>("config.runtime");
    if (!runtimeConfig) {
        throw std::runtime_error("JsonConfigService::LoadFromJson: workflow did not produce runtime config");
    }

    return *runtimeConfig;
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

    rapidjson::Value windowObject(rapidjson::kObjectType);
    addStringMember(windowObject, "title", config.windowTitle);
    rapidjson::Value sizeObject(rapidjson::kObjectType);
    sizeObject.AddMember("width", config.width, allocator);
    sizeObject.AddMember("height", config.height, allocator);
    windowObject.AddMember("size", sizeObject, allocator);

    std::filesystem::path projectRoot = config.projectRoot;
    if (projectRoot.empty()) {
        projectRoot = configPath.empty() ? std::filesystem::current_path() : configPath.parent_path();
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

    rapidjson::Value pathsObject(rapidjson::kObjectType);
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
