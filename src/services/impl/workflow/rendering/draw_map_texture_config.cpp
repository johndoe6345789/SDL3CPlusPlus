#include "services/interfaces/workflow/rendering/draw_map_texture_config.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <algorithm>
#include <cctype>

namespace sdl3cpp::services::impl {

namespace {

std::string ToLower(std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

}  // namespace

bool IsPortalTexture(const std::string& textureName) {
    const std::string lower = ToLower(textureName);
    return lower.find("portal_sfx") != std::string::npos ||
           lower.find("mapobjects/portal") != std::string::npos;
}

DrawMapTextureConfig ReadDrawMapTextureConfig(
    const WorkflowStepDefinition& step) {
    DrawMapTextureConfig config;

    for (const auto& [key, param] : step.parameters) {
        if (key == "roughness" || key == "metallic" || key == "pipeline_key") {
            continue;
        }
        if (param.type == WorkflowParameterValue::Type::String) {
            if (key == "default_texture") {
                config.defaultTexture = param.stringValue;
            } else {
                config.mappings.emplace_back(key, param.stringValue);
            }
        }
    }
    if (config.defaultTexture.empty() && !config.mappings.empty()) {
        config.defaultTexture = config.mappings.back().second;
    }

    WorkflowStepParameterResolver params;
    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<float>(p->numberValue)
                   : def;
    };
    config.roughness = getNum("roughness", 0.8f);
    config.metallic  = getNum("metallic", 0.0f);
    return config;
}

std::string ResolveLegacyMeshTexture(const std::string& meshName,
                                     const DrawMapTextureConfig& config) {
    std::string texKey = config.defaultTexture;
    for (const auto& [pattern, texName] : config.mappings) {
        if (meshName.find(pattern) != std::string::npos) {
            texKey = texName;
            break;
        }
    }
    return texKey;
}

}  // namespace sdl3cpp::services::impl
