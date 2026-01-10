#include "materialx_shader_system.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

MaterialXShaderSystem::MaterialXShaderSystem(std::shared_ptr<IConfigService> configService,
                                             std::shared_ptr<ILogger> logger)
    : configService_(std::move(configService)),
      logger_(std::move(logger)),
      materialxGenerator_(logger_) {
    if (logger_) {
        logger_->Trace("MaterialXShaderSystem", "MaterialXShaderSystem",
                       "configService=" + std::string(configService_ ? "set" : "null"));
    }
}

std::unordered_map<std::string, ShaderPaths> MaterialXShaderSystem::BuildShaderMap() {
    if (logger_) {
        logger_->Trace("MaterialXShaderSystem", "BuildShaderMap",
                       "Generating MaterialX shaders from config");
    }

    if (!configService_) {
        throw std::runtime_error("MaterialXShaderSystem requires a config service");
    }

    std::unordered_map<std::string, ShaderPaths> shaderMap;
    const auto& materialConfig = configService_->GetMaterialXConfig();
    const auto& materialOverrides = configService_->GetMaterialXMaterialConfigs();

    if (logger_) {
        logger_->Trace("MaterialXShaderSystem", "BuildShaderMap",
                       "materialOverrides=" + std::to_string(materialOverrides.size()) +
                           ", baseEnabled=" + std::string(materialConfig.enabled ? "true" : "false"));
    }

    const auto scriptDirectory = configService_
        ? configService_->GetProjectRoot()
        : std::filesystem::path{};

    const auto addShader = [&](const MaterialXConfig& config, const std::string& sourceLabel) {
        if (!config.enabled) {
            if (logger_) {
                logger_->Trace("MaterialXShaderSystem", "BuildShaderMap",
                               "Skipping MaterialX shader generation (disabled) source=" + sourceLabel);
            }
            return;
        }
        if (config.shaderKey.empty()) {
            if (logger_) {
                logger_->Error("MaterialX shader generation requires a shaderKey (source=" + sourceLabel + ")");
            }
            return;
        }
        if (config.documentPath.empty() && !config.useConstantColor) {
            if (logger_) {
                logger_->Trace("MaterialXShaderSystem", "BuildShaderMap",
                               "Skipping MaterialX shader generation (no document/constant color) key=" +
                                   config.shaderKey + ", source=" + sourceLabel);
            }
            return;
        }
        if (shaderMap.find(config.shaderKey) != shaderMap.end()) {
            if (logger_) {
                logger_->Trace("MaterialXShaderSystem", "BuildShaderMap",
                               "Skipping MaterialX shader generation (duplicate key) key=" +
                                   config.shaderKey + ", source=" + sourceLabel);
            }
            return;
        }
        if (logger_) {
            logger_->Trace("MaterialXShaderSystem", "BuildShaderMap",
                           "Generating MaterialX shader key=" + config.shaderKey +
                               ", document=" + config.documentPath.string() +
                               ", material=" + config.materialName +
                               ", source=" + sourceLabel);
        }
        try {
            ShaderPaths materialShader = materialxGenerator_.Generate(config, scriptDirectory);
            shaderMap[config.shaderKey] = std::move(materialShader);
        } catch (const std::exception& ex) {
            if (logger_) {
                logger_->Error("MaterialX shader generation failed for key=" +
                               config.shaderKey + ": " + std::string(ex.what()));
            }
        }
    };

    for (const auto& overrideConfig : materialOverrides) {
        if (!overrideConfig.enabled) {
            continue;
        }
        MaterialXConfig resolvedConfig = materialConfig;
        resolvedConfig.enabled = true;
        resolvedConfig.documentPath = overrideConfig.documentPath;
        resolvedConfig.shaderKey = overrideConfig.shaderKey;
        resolvedConfig.materialName = overrideConfig.materialName;
        resolvedConfig.useConstantColor = overrideConfig.useConstantColor;
        resolvedConfig.constantColor = overrideConfig.constantColor;
        addShader(resolvedConfig, "materialx_materials");
    }

    addShader(materialConfig, "materialx");

    if (shaderMap.empty()) {
        if (logger_) {
            logger_->Error("No MaterialX shaders were generated from JSON config");
        }
        throw std::runtime_error("No MaterialX shaders were generated from JSON config");
    }

    lastShaderMap_ = shaderMap;
    return shaderMap;
}

ShaderReflection MaterialXShaderSystem::GetReflection(const std::string& shaderKey) const {
    if (logger_) {
        logger_->Trace("MaterialXShaderSystem", "GetReflection", "shaderKey=" + shaderKey);
    }
    ShaderReflection reflection;
    auto it = lastShaderMap_.find(shaderKey);
    if (it == lastShaderMap_.end()) {
        return reflection;
    }
    reflection.textures.reserve(it->second.textures.size());
    for (const auto& binding : it->second.textures) {
        reflection.textures.push_back(binding.uniformName);
    }
    return reflection;
}

std::vector<ShaderPaths::TextureBinding> MaterialXShaderSystem::GetDefaultTextures(
    const std::string& shaderKey) const {
    if (logger_) {
        logger_->Trace("MaterialXShaderSystem", "GetDefaultTextures", "shaderKey=" + shaderKey);
    }
    auto it = lastShaderMap_.find(shaderKey);
    if (it == lastShaderMap_.end()) {
        return {};
    }
    return it->second.textures;
}

}  // namespace sdl3cpp::services::impl
