#include "config_compiler_service.hpp"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace sdl3cpp::services::impl {

namespace {

bool IsErrorSeverity(ProbeSeverity severity) {
    return severity == ProbeSeverity::Error || severity == ProbeSeverity::Fatal;
}

std::string JoinPath(const std::string& base, const std::string& segment) {
    if (base.empty()) {
        return "/" + segment;
    }
    return base + "/" + segment;
}

bool ParsePassOutputReference(const std::string& value, std::string& passId, std::string& outputName) {
    const std::string prefix = "@pass.";
    if (value.rfind(prefix, 0) != 0) {
        return false;
    }
    const std::string remainder = value.substr(prefix.size());
    const size_t dot = remainder.find('.');
    if (dot == std::string::npos) {
        return false;
    }
    passId = remainder.substr(0, dot);
    outputName = remainder.substr(dot + 1);
    return !passId.empty() && !outputName.empty();
}

}  // namespace

ConfigCompilerService::ConfigCompilerService(std::shared_ptr<IConfigService> configService,
                                             std::shared_ptr<IRenderGraphService> renderGraphService,
                                             std::shared_ptr<IProbeService> probeService,
                                             std::shared_ptr<ILogger> logger)
    : configService_(std::move(configService)),
      renderGraphService_(std::move(renderGraphService)),
      probeService_(std::move(probeService)),
      logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("ConfigCompilerService", "ConfigCompilerService", "initialized=true");
    }
}

void ConfigCompilerService::Initialize() {
    if (!configService_) {
        throw std::runtime_error("ConfigCompilerService requires a config service");
    }
    const std::string configJson = configService_->GetConfigJson();
    if (configJson.empty()) {
        if (logger_) {
            logger_->Warn("ConfigCompilerService::Initialize: Config JSON is empty; skipping compile");
        }
        return;
    }
    lastResult_ = Compile(configJson);
    if (logger_) {
        const std::string status = lastResult_.success ? "success" : "errors";
        logger_->Info("ConfigCompilerService::Initialize: Config compile " + status +
                      " (diagnostics=" + std::to_string(lastResult_.diagnostics.size()) + ")");
    }
}

ConfigCompilerResult ConfigCompilerService::Compile(const std::string& configJson) {
    ConfigCompilerResult result;
    result.success = true;

    rapidjson::Document document;
    document.Parse(configJson.c_str());
    if (document.HasParseError()) {
        AddDiagnostic(result,
                      ProbeSeverity::Error,
                      "CONFIG_JSON_PARSE",
                      "",
                      std::string("JSON parse error: ") + rapidjson::GetParseError_En(document.GetParseError()),
                      "offset=" + std::to_string(document.GetErrorOffset()));
        return result;
    }
    if (!document.IsObject()) {
        AddDiagnostic(result,
                      ProbeSeverity::Error,
                      "CONFIG_JSON_ROOT",
                      "",
                      "JSON root must be an object");
        return result;
    }

    auto getObjectMember = [&](const rapidjson::Value& parent,
                               const char* name,
                               const std::string& path) -> const rapidjson::Value* {
        if (!parent.HasMember(name)) {
            return nullptr;
        }
        const auto& value = parent[name];
        if (!value.IsObject()) {
            AddDiagnostic(result,
                          ProbeSeverity::Error,
                          "CONFIG_JSON_TYPE",
                          path,
                          std::string("Expected object for ") + name);
            return nullptr;
        }
        return &value;
    };

    auto getArrayMember = [&](const rapidjson::Value& parent,
                              const char* name,
                              const std::string& path) -> const rapidjson::Value* {
        if (!parent.HasMember(name)) {
            return nullptr;
        }
        const auto& value = parent[name];
        if (!value.IsArray()) {
            AddDiagnostic(result,
                          ProbeSeverity::Error,
                          "CONFIG_JSON_TYPE",
                          path,
                          std::string("Expected array for ") + name);
            return nullptr;
        }
        return &value;
    };

    const std::string scenePath = "/scene";
    if (const auto* sceneValue = getObjectMember(document, "scene", scenePath)) {
        const std::string entitiesPath = JoinPath(scenePath, "entities");
        if (const auto* entitiesValue = getArrayMember(*sceneValue, "entities", entitiesPath)) {
            for (rapidjson::SizeType i = 0; i < entitiesValue->Size(); ++i) {
                const auto& entry = (*entitiesValue)[i];
                const std::string entityPath = entitiesPath + "/" + std::to_string(i);
                if (!entry.IsObject()) {
                    AddDiagnostic(result,
                                  ProbeSeverity::Error,
                                  "SCENE_ENTITY_TYPE",
                                  entityPath,
                                  "Scene entity must be an object");
                    continue;
                }
                SceneEntityIR entity;
                entity.jsonPath = entityPath;
                if (entry.HasMember("id") && entry["id"].IsString()) {
                    entity.id = entry["id"].GetString();
                } else {
                    AddDiagnostic(result,
                                  ProbeSeverity::Error,
                                  "SCENE_ENTITY_ID_MISSING",
                                  entityPath,
                                  "Scene entity requires a string id");
                    continue;
                }
                if (entry.HasMember("components")) {
                    if (!entry["components"].IsObject()) {
                        AddDiagnostic(result,
                                      ProbeSeverity::Error,
                                      "SCENE_COMPONENTS_TYPE",
                                      JoinPath(entityPath, "components"),
                                      "Scene components must be an object");
                    } else {
                        const auto& components = entry["components"];
                        for (auto it = components.MemberBegin(); it != components.MemberEnd(); ++it) {
                            if (it->name.IsString()) {
                                entity.componentTypes.emplace_back(it->name.GetString());
                            }
                        }
                    }
                }
                result.scene.entities.push_back(std::move(entity));
            }
        }
    }

    const std::string assetsPath = "/assets";
    std::unordered_set<std::string> textureIds;
    std::unordered_set<std::string> shaderIds;
    std::unordered_map<std::string, bool> shaderSystems;
    std::string activeShaderSystem;
    const std::string shaderSystemsPath = "/shader_systems";
    if (document.HasMember("shader_systems")) {
        const auto& shaderSystemsValue = document["shader_systems"];
        if (!shaderSystemsValue.IsObject()) {
            AddDiagnostic(result,
                          ProbeSeverity::Error,
                          "SHADER_SYSTEMS_TYPE",
                          shaderSystemsPath,
                          "shader_systems must be an object");
        } else {
            if (shaderSystemsValue.HasMember("active")) {
                const auto& activeValue = shaderSystemsValue["active"];
                if (!activeValue.IsString()) {
                    AddDiagnostic(result,
                                  ProbeSeverity::Error,
                                  "SHADER_SYSTEMS_ACTIVE_TYPE",
                                  JoinPath(shaderSystemsPath, "active"),
                                  "shader_systems.active must be a string");
                } else {
                    activeShaderSystem = activeValue.GetString();
                }
            }
            if (shaderSystemsValue.HasMember("systems")) {
                const auto& systemsValue = shaderSystemsValue["systems"];
                const std::string systemsPath = JoinPath(shaderSystemsPath, "systems");
                if (!systemsValue.IsObject()) {
                    AddDiagnostic(result,
                                  ProbeSeverity::Error,
                                  "SHADER_SYSTEMS_ENTRIES_TYPE",
                                  systemsPath,
                                  "shader_systems.systems must be an object");
                } else {
                    for (auto it = systemsValue.MemberBegin(); it != systemsValue.MemberEnd(); ++it) {
                        const std::string systemId = it->name.GetString();
                        const std::string systemPath = JoinPath(systemsPath, systemId);
                        bool enabled = true;
                        if (!it->value.IsObject()) {
                            AddDiagnostic(result,
                                          ProbeSeverity::Error,
                                          "SHADER_SYSTEMS_ENTRY_TYPE",
                                          systemPath,
                                          "shader_systems.systems entries must be objects");
                            continue;
                        }
                        if (it->value.HasMember("enabled")) {
                            const auto& enabledValue = it->value["enabled"];
                            if (!enabledValue.IsBool()) {
                                AddDiagnostic(result,
                                              ProbeSeverity::Error,
                                              "SHADER_SYSTEMS_ENABLED_TYPE",
                                              JoinPath(systemPath, "enabled"),
                                              "shader_systems.systems.enabled must be a boolean");
                            } else {
                                enabled = enabledValue.GetBool();
                            }
                        }
                        shaderSystems.emplace(systemId, enabled);
                    }
                }
            }
        }
    }
    if (!activeShaderSystem.empty() && !shaderSystems.empty()) {
        auto activeIt = shaderSystems.find(activeShaderSystem);
        if (activeIt == shaderSystems.end()) {
            AddDiagnostic(result,
                          ProbeSeverity::Error,
                          "SHADER_SYSTEMS_ACTIVE_UNKNOWN",
                          JoinPath(shaderSystemsPath, "active"),
                          "shader_systems.active is not declared in shader_systems.systems");
        } else if (!activeIt->second) {
            AddDiagnostic(result,
                          ProbeSeverity::Warn,
                          "SHADER_SYSTEMS_ACTIVE_DISABLED",
                          JoinPath(shaderSystemsPath, "active"),
                          "shader_systems.active references a disabled shader system");
        }
    }
    if (const auto* assetsValue = getObjectMember(document, "assets", assetsPath)) {
        if (const auto* texturesValue = getObjectMember(*assetsValue, "textures", JoinPath(assetsPath, "textures"))) {
            for (auto it = texturesValue->MemberBegin(); it != texturesValue->MemberEnd(); ++it) {
                const std::string id = it->name.GetString();
                const std::string texturePath = JoinPath(JoinPath(assetsPath, "textures"), id);
                if (!it->value.IsObject()) {
                    AddDiagnostic(result,
                                  ProbeSeverity::Error,
                                  "ASSET_TEXTURE_TYPE",
                                  texturePath,
                                  "Texture entry must be an object");
                    continue;
                }
                if (!it->value.HasMember("uri") || !it->value["uri"].IsString()) {
                    AddDiagnostic(result,
                                  ProbeSeverity::Error,
                                  "ASSET_TEXTURE_URI",
                                  JoinPath(texturePath, "uri"),
                                  "Texture entry requires a string uri");
                    continue;
                }
                TextureIR texture;
                texture.id = id;
                texture.uri = it->value["uri"].GetString();
                texture.jsonPath = texturePath;
                result.resources.textures.push_back(std::move(texture));
                textureIds.insert(id);
            }
        }

        if (const auto* meshesValue = getObjectMember(*assetsValue, "meshes", JoinPath(assetsPath, "meshes"))) {
            for (auto it = meshesValue->MemberBegin(); it != meshesValue->MemberEnd(); ++it) {
                const std::string id = it->name.GetString();
                const std::string meshPath = JoinPath(JoinPath(assetsPath, "meshes"), id);
                if (!it->value.IsObject()) {
                    AddDiagnostic(result,
                                  ProbeSeverity::Error,
                                  "ASSET_MESH_TYPE",
                                  meshPath,
                                  "Mesh entry must be an object");
                    continue;
                }
                if (!it->value.HasMember("uri") || !it->value["uri"].IsString()) {
                    AddDiagnostic(result,
                                  ProbeSeverity::Error,
                                  "ASSET_MESH_URI",
                                  JoinPath(meshPath, "uri"),
                                  "Mesh entry requires a string uri");
                    continue;
                }
                MeshIR mesh;
                mesh.id = id;
                mesh.uri = it->value["uri"].GetString();
                mesh.jsonPath = meshPath;
                result.resources.meshes.push_back(std::move(mesh));
            }
        }

        if (const auto* shadersValue = getObjectMember(*assetsValue, "shaders", JoinPath(assetsPath, "shaders"))) {
            for (auto it = shadersValue->MemberBegin(); it != shadersValue->MemberEnd(); ++it) {
                const std::string id = it->name.GetString();
                const std::string shaderPath = JoinPath(JoinPath(assetsPath, "shaders"), id);
                if (!it->value.IsObject()) {
                    AddDiagnostic(result,
                                  ProbeSeverity::Error,
                                  "ASSET_SHADER_TYPE",
                                  shaderPath,
                                  "Shader entry must be an object");
                    continue;
                }
                if (!it->value.HasMember("vs") || !it->value["vs"].IsString() ||
                    !it->value.HasMember("fs") || !it->value["fs"].IsString()) {
                    AddDiagnostic(result,
                                  ProbeSeverity::Error,
                                  "ASSET_SHADER_PATHS",
                                  shaderPath,
                                  "Shader entry requires vs and fs string paths");
                    continue;
                }
                ShaderIR shader;
                shader.id = id;
                shader.vertexPath = it->value["vs"].GetString();
                shader.fragmentPath = it->value["fs"].GetString();
                bool systemValid = true;
                if (it->value.HasMember("system")) {
                    const auto& systemValue = it->value["system"];
                    if (!systemValue.IsString()) {
                        AddDiagnostic(result,
                                      ProbeSeverity::Error,
                                      "ASSET_SHADER_SYSTEM_TYPE",
                                      JoinPath(shaderPath, "system"),
                                      "Shader system must be a string");
                        systemValid = false;
                    } else {
                        shader.system = systemValue.GetString();
                    }
                }
                if (!systemValid) {
                    continue;
                }
                if (!shader.system.empty() && !shaderSystems.empty()) {
                    auto systemIt = shaderSystems.find(shader.system);
                    if (systemIt == shaderSystems.end()) {
                        AddDiagnostic(result,
                                      ProbeSeverity::Error,
                                      "ASSET_SHADER_SYSTEM_UNKNOWN",
                                      JoinPath(shaderPath, "system"),
                                      "Shader system is not declared in shader_systems.systems");
                        continue;
                    }
                    if (!systemIt->second) {
                        AddDiagnostic(result,
                                      ProbeSeverity::Warn,
                                      "ASSET_SHADER_SYSTEM_DISABLED",
                                      JoinPath(shaderPath, "system"),
                                      "Shader system is disabled in shader_systems.systems");
                    }
                }
                shader.jsonPath = shaderPath;
                result.resources.shaders.push_back(std::move(shader));
                shaderIds.insert(id);
            }
        }
    }

    const std::string materialsPath = "/materials";
    if (const auto* materialsValue = getObjectMember(document, "materials", materialsPath)) {
        for (auto it = materialsValue->MemberBegin(); it != materialsValue->MemberEnd(); ++it) {
            const std::string id = it->name.GetString();
            const std::string materialPath = JoinPath(materialsPath, id);
            if (!it->value.IsObject()) {
                AddDiagnostic(result,
                              ProbeSeverity::Error,
                              "MATERIAL_TYPE",
                              materialPath,
                              "Material entry must be an object");
                continue;
            }
            if (!it->value.HasMember("shader") || !it->value["shader"].IsString()) {
                AddDiagnostic(result,
                              ProbeSeverity::Error,
                              "MATERIAL_SHADER",
                              JoinPath(materialPath, "shader"),
                              "Material requires a shader id");
                continue;
            }
            MaterialIR material;
            material.id = id;
            material.shader = it->value["shader"].GetString();
            material.jsonPath = materialPath;

            if (!shaderIds.empty() && shaderIds.find(material.shader) == shaderIds.end()) {
                AddDiagnostic(result,
                              ProbeSeverity::Error,
                              "MATERIAL_SHADER_UNKNOWN",
                              JoinPath(materialPath, "shader"),
                              "Material references unknown shader: " + material.shader);
            }

            if (it->value.HasMember("textures")) {
                const auto& texturesValue = it->value["textures"];
                if (!texturesValue.IsObject()) {
                    AddDiagnostic(result,
                                  ProbeSeverity::Error,
                                  "MATERIAL_TEXTURES_TYPE",
                                  JoinPath(materialPath, "textures"),
                                  "Material textures must be an object");
                } else {
                    for (auto texIt = texturesValue.MemberBegin(); texIt != texturesValue.MemberEnd(); ++texIt) {
                        const std::string uniform = texIt->name.GetString();
                        if (!texIt->value.IsString()) {
                            AddDiagnostic(result,
                                          ProbeSeverity::Error,
                                          "MATERIAL_TEXTURE_REF",
                                          JoinPath(JoinPath(materialPath, "textures"), uniform),
                                          "Material texture mapping must be a string id");
                            continue;
                        }
                        const std::string textureId = texIt->value.GetString();
                        material.textures.emplace(uniform, textureId);
                        if (!textureIds.empty() && textureIds.find(textureId) == textureIds.end()) {
                            AddDiagnostic(result,
                                          ProbeSeverity::Error,
                                          "MATERIAL_TEXTURE_UNKNOWN",
                                          JoinPath(JoinPath(materialPath, "textures"), uniform),
                                          "Material references unknown texture: " + textureId);
                        }
                    }
                }
            }

            result.resources.materials.push_back(std::move(material));
        }
    }

    const std::string renderPath = "/render";
    if (const auto* renderValue = getObjectMember(document, "render", renderPath)) {
        const std::string passesPath = JoinPath(renderPath, "passes");
        if (const auto* passesValue = getArrayMember(*renderValue, "passes", passesPath)) {
            std::unordered_map<std::string, std::unordered_set<std::string>> outputsByPass;
            for (rapidjson::SizeType i = 0; i < passesValue->Size(); ++i) {
                const auto& passValue = (*passesValue)[i];
                const std::string passPath = passesPath + "/" + std::to_string(i);
                if (!passValue.IsObject()) {
                    AddDiagnostic(result,
                                  ProbeSeverity::Error,
                                  "RG_PASS_TYPE",
                                  passPath,
                                  "Render pass must be an object");
                    continue;
                }
                RenderPassIR pass;
                pass.jsonPath = passPath;
                if (passValue.HasMember("id") && passValue["id"].IsString()) {
                    pass.id = passValue["id"].GetString();
                } else {
                    AddDiagnostic(result,
                                  ProbeSeverity::Error,
                                  "RG_PASS_ID",
                                  JoinPath(passPath, "id"),
                                  "Render pass requires a string id");
                    continue;
                }
                if (passValue.HasMember("type") && passValue["type"].IsString()) {
                    pass.type = passValue["type"].GetString();
                }
                if (passValue.HasMember("view_id")) {
                    const auto& viewValue = passValue["view_id"];
                    const std::string viewPath = JoinPath(passPath, "view_id");
                    if (!viewValue.IsInt()) {
                        AddDiagnostic(result,
                                      ProbeSeverity::Error,
                                      "RG_VIEW_ID_TYPE",
                                      viewPath,
                                      "Render pass view_id must be an integer");
                    } else if (viewValue.GetInt() < 0) {
                        AddDiagnostic(result,
                                      ProbeSeverity::Error,
                                      "RG_VIEW_ID_RANGE",
                                      viewPath,
                                      "Render pass view_id must be zero or greater");
                    } else {
                        pass.hasViewId = true;
                        pass.viewId = viewValue.GetInt();
                    }
                }
                if (passValue.HasMember("clear")) {
                    const auto& clearValue = passValue["clear"];
                    const std::string clearPath = JoinPath(passPath, "clear");
                    if (!clearValue.IsObject()) {
                        AddDiagnostic(result,
                                      ProbeSeverity::Error,
                                      "RG_CLEAR_TYPE",
                                      clearPath,
                                      "Render pass clear must be an object");
                    } else {
                        RenderPassClearIR clear;
                        clear.enabled = true;
                        clear.jsonPath = clearPath;
                        bool hasFlags = false;
                        bool hasColor = false;
                        bool hasDepth = false;
                        bool hasStencil = false;
                        if (clearValue.HasMember("flags")) {
                            const auto& flagsValue = clearValue["flags"];
                            const std::string flagsPath = JoinPath(clearPath, "flags");
                            if (!flagsValue.IsArray()) {
                                AddDiagnostic(result,
                                              ProbeSeverity::Error,
                                              "RG_CLEAR_FLAGS_TYPE",
                                              flagsPath,
                                              "Render pass clear flags must be an array");
                            } else {
                                hasFlags = true;
                                for (rapidjson::SizeType f = 0; f < flagsValue.Size(); ++f) {
                                    const auto& flagValue = flagsValue[f];
                                    if (!flagValue.IsString()) {
                                        AddDiagnostic(result,
                                                      ProbeSeverity::Error,
                                                      "RG_CLEAR_FLAGS_VALUE",
                                                      flagsPath,
                                                      "Render pass clear flag entries must be strings");
                                        continue;
                                    }
                                    const std::string flagName = flagValue.GetString();
                                    if (flagName == "color") {
                                        clear.clearColor = true;
                                    } else if (flagName == "depth") {
                                        clear.clearDepth = true;
                                    } else if (flagName == "stencil") {
                                        clear.clearStencil = true;
                                    } else {
                                        AddDiagnostic(result,
                                                      ProbeSeverity::Error,
                                                      "RG_CLEAR_FLAGS_VALUE",
                                                      flagsPath,
                                                      "Render pass clear flag must be color, depth, or stencil");
                                    }
                                }
                            }
                        }
                        if (clearValue.HasMember("color")) {
                            const auto& colorValue = clearValue["color"];
                            const std::string colorPath = JoinPath(clearPath, "color");
                            if (!colorValue.IsArray() || colorValue.Size() != 4) {
                                AddDiagnostic(result,
                                              ProbeSeverity::Error,
                                              "RG_CLEAR_COLOR",
                                              colorPath,
                                              "Render pass clear color must be a 4-element array");
                            } else {
                                bool colorValid = true;
                                for (rapidjson::SizeType c = 0; c < colorValue.Size(); ++c) {
                                    if (!colorValue[c].IsNumber()) {
                                        colorValid = false;
                                        break;
                                    }
                                }
                                if (!colorValid) {
                                    AddDiagnostic(result,
                                                  ProbeSeverity::Error,
                                                  "RG_CLEAR_COLOR",
                                                  colorPath,
                                                  "Render pass clear color values must be numbers");
                                } else {
                                    for (rapidjson::SizeType c = 0; c < colorValue.Size(); ++c) {
                                        clear.color[c] = static_cast<float>(colorValue[c].GetDouble());
                                    }
                                    hasColor = true;
                                }
                            }
                        }
                        if (clearValue.HasMember("depth")) {
                            const auto& depthValue = clearValue["depth"];
                            const std::string depthPath = JoinPath(clearPath, "depth");
                            if (!depthValue.IsNumber()) {
                                AddDiagnostic(result,
                                              ProbeSeverity::Error,
                                              "RG_CLEAR_DEPTH",
                                              depthPath,
                                              "Render pass clear depth must be a number");
                            } else {
                                clear.depth = static_cast<float>(depthValue.GetDouble());
                                hasDepth = true;
                            }
                        }
                        if (clearValue.HasMember("stencil")) {
                            const auto& stencilValue = clearValue["stencil"];
                            const std::string stencilPath = JoinPath(clearPath, "stencil");
                            if (!stencilValue.IsInt()) {
                                AddDiagnostic(result,
                                              ProbeSeverity::Error,
                                              "RG_CLEAR_STENCIL",
                                              stencilPath,
                                              "Render pass clear stencil must be an integer");
                            } else if (stencilValue.GetInt() < 0) {
                                AddDiagnostic(result,
                                              ProbeSeverity::Error,
                                              "RG_CLEAR_STENCIL",
                                              stencilPath,
                                              "Render pass clear stencil must be zero or greater");
                            } else {
                                clear.stencil = stencilValue.GetInt();
                                hasStencil = true;
                            }
                        }

                        if (!hasFlags) {
                            if (hasColor) {
                                clear.clearColor = true;
                            }
                            if (hasDepth) {
                                clear.clearDepth = true;
                            }
                            if (hasStencil) {
                                clear.clearStencil = true;
                            }
                        }

                        pass.clear = std::move(clear);
                    }
                }
                outputsByPass.emplace(pass.id, std::unordered_set<std::string>{});

                if (passValue.HasMember("inputs")) {
                    const auto& inputsValue = passValue["inputs"];
                    if (!inputsValue.IsObject()) {
                        AddDiagnostic(result,
                                      ProbeSeverity::Error,
                                      "RG_INPUTS_TYPE",
                                      JoinPath(passPath, "inputs"),
                                      "Render pass inputs must be an object");
                    } else {
                        for (auto inputIt = inputsValue.MemberBegin(); inputIt != inputsValue.MemberEnd(); ++inputIt) {
                            const std::string inputName = inputIt->name.GetString();
                            const std::string inputPath = JoinPath(JoinPath(passPath, "inputs"), inputName);
                            if (!inputIt->value.IsString()) {
                                AddDiagnostic(result,
                                              ProbeSeverity::Error,
                                              "RG_INPUT_REF",
                                              inputPath,
                                              "Render pass input must be a string reference");
                                continue;
                            }
                            const std::string inputValue = inputIt->value.GetString();
                            RenderPassInputIR input;
                            input.name = inputName;
                            input.jsonPath = inputPath;
                            if (inputValue == "@swapchain") {
                                AddDiagnostic(result,
                                              ProbeSeverity::Error,
                                              "RG_INPUT_SWAPCHAIN",
                                              inputPath,
                                              "Render pass input cannot reference @swapchain");
                            } else {
                                std::string passId;
                                std::string outputName;
                                if (ParsePassOutputReference(inputValue, passId, outputName)) {
                                    input.ref.type = RenderResourceRefType::PassOutput;
                                    input.ref.passId = passId;
                                    input.ref.outputName = outputName;
                                    input.ref.jsonPath = inputPath;
                                } else {
                                    AddDiagnostic(result,
                                                  ProbeSeverity::Error,
                                                  "RG_INPUT_REF_FORMAT",
                                                  inputPath,
                                                  "Render pass input must use @pass.<id>.<output>");
                                }
                            }
                            pass.inputs.push_back(std::move(input));
                        }
                    }
                }

                if (passValue.HasMember("outputs")) {
                    const auto& outputsValue = passValue["outputs"];
                    if (!outputsValue.IsObject()) {
                        AddDiagnostic(result,
                                      ProbeSeverity::Error,
                                      "RG_OUTPUTS_TYPE",
                                      JoinPath(passPath, "outputs"),
                                      "Render pass outputs must be an object");
                    } else {
                        for (auto outputIt = outputsValue.MemberBegin(); outputIt != outputsValue.MemberEnd(); ++outputIt) {
                            const std::string outputName = outputIt->name.GetString();
                            const std::string outputPath = JoinPath(JoinPath(passPath, "outputs"), outputName);
                            RenderPassOutputIR output;
                            output.name = outputName;
                            output.jsonPath = outputPath;
                            bool outputValid = true;
                            if (outputIt->value.IsString()) {
                                const std::string outputValue = outputIt->value.GetString();
                                if (outputValue == "@swapchain") {
                                    output.isSwapchain = true;
                                } else {
                                    AddDiagnostic(result,
                                                  ProbeSeverity::Error,
                                                  "RG_OUTPUT_REF",
                                                  outputPath,
                                                  "Render pass output must be an object or @swapchain");
                                    outputValid = false;
                                }
                            } else if (outputIt->value.IsObject()) {
                                const auto& outputObject = outputIt->value;
                                if (outputObject.HasMember("format") && outputObject["format"].IsString()) {
                                    output.format = outputObject["format"].GetString();
                                }
                                if (outputObject.HasMember("usage") && outputObject["usage"].IsString()) {
                                    output.usage = outputObject["usage"].GetString();
                                }
                            } else {
                                AddDiagnostic(result,
                                              ProbeSeverity::Error,
                                              "RG_OUTPUT_TYPE",
                                              outputPath,
                                              "Render pass output must be an object or string");
                                outputValid = false;
                            }
                            if (outputValid) {
                                pass.outputs.push_back(std::move(output));
                                if (!pass.id.empty() && !output.isSwapchain) {
                                    outputsByPass[pass.id].insert(outputName);
                                }
                            }
                        }
                    }
                }

                result.renderGraph.passes.push_back(std::move(pass));
            }

            for (const auto& pass : result.renderGraph.passes) {
                for (const auto& input : pass.inputs) {
                    if (input.ref.type != RenderResourceRefType::PassOutput) {
                        continue;
                    }
                    if (input.ref.passId == pass.id) {
                        AddDiagnostic(result,
                                      ProbeSeverity::Error,
                                      "RG_INPUT_SELF_REFERENCE",
                                      input.jsonPath,
                                      "Render pass input references its own output");
                        continue;
                    }
                    auto passIt = outputsByPass.find(input.ref.passId);
                    if (passIt == outputsByPass.end()) {
                        AddDiagnostic(result,
                                      ProbeSeverity::Error,
                                      "RG_INPUT_UNKNOWN_PASS",
                                      input.jsonPath,
                                      "Render pass input references unknown pass: " + input.ref.passId);
                        continue;
                    }
                    if (!input.ref.outputName.empty() &&
                        passIt->second.find(input.ref.outputName) == passIt->second.end()) {
                        AddDiagnostic(result,
                                      ProbeSeverity::Error,
                                      "RG_INPUT_UNKNOWN_OUTPUT",
                                      input.jsonPath,
                                      "Render pass input references unknown output: " + input.ref.outputName);
                    }
                }
            }
        }
    }

    if (renderGraphService_) {
        const auto buildResult = renderGraphService_->BuildGraph(result.renderGraph);
        MergeRenderGraphDiagnostics(result, buildResult);
    }

    if (logger_) {
        logger_->Trace("ConfigCompilerService", "Compile",
                       "diagnostics=" + std::to_string(result.diagnostics.size()) +
                       ", success=" + std::string(result.success ? "true" : "false"));
    }

    return result;
}

void ConfigCompilerService::AddDiagnostic(ConfigCompilerResult& result,
                                          ProbeSeverity severity,
                                          const std::string& code,
                                          const std::string& jsonPath,
                                          const std::string& message,
                                          const std::string& details) {
    ProbeReport report;
    report.severity = severity;
    report.code = code;
    report.jsonPath = jsonPath;
    report.message = message;
    report.details = details;
    result.diagnostics.push_back(report);
    if (IsErrorSeverity(severity)) {
        result.success = false;
    }
    if (probeService_) {
        probeService_->Report(report);
    }
    if (logger_) {
        if (severity == ProbeSeverity::Warn) {
            logger_->Warn("ConfigCompilerService::Compile: " + message + " (" + code + ")");
        } else if (IsErrorSeverity(severity)) {
            logger_->Error("ConfigCompilerService::Compile: " + message + " (" + code + ")");
        } else {
            logger_->Info("ConfigCompilerService::Compile: " + message + " (" + code + ")");
        }
    }
}

void ConfigCompilerService::MergeRenderGraphDiagnostics(ConfigCompilerResult& result,
                                                        const RenderGraphBuildResult& renderGraphBuild) {
    result.renderGraphBuild = renderGraphBuild;
    for (const auto& report : renderGraphBuild.diagnostics) {
        result.diagnostics.push_back(report);
        if (IsErrorSeverity(report.severity)) {
            result.success = false;
        }
    }
}

}  // namespace sdl3cpp::services::impl
