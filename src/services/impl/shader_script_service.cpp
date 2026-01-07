#include "shader_script_service.hpp"

#include "services/interfaces/i_logger.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace sdl3cpp::services::impl {

ShaderScriptService::ShaderScriptService(std::shared_ptr<IScriptEngineService> engineService,
                                         std::shared_ptr<IConfigService> configService,
                                         std::shared_ptr<ILogger> logger)
    : engineService_(std::move(engineService)),
      configService_(std::move(configService)),
      logger_(std::move(logger)),
      materialxGenerator_(logger_) {
    if (logger_) {
        logger_->Trace("ShaderScriptService", "ShaderScriptService",
                       "engineService=" + std::string(engineService_ ? "set" : "null") +
                       ", configService=" + std::string(configService_ ? "set" : "null"));
    }
}

std::unordered_map<std::string, ShaderPaths> ShaderScriptService::LoadShaderPathsMap() {
    if (logger_) {
        logger_->Trace("ShaderScriptService", "LoadShaderPathsMap",
                       "Generating MaterialX shaders from JSON config only");
    }

    std::unordered_map<std::string, ShaderPaths> shaderMap;

    if (configService_) {
        const auto& materialConfig = configService_->GetMaterialXConfig();
        const auto& materialOverrides = configService_->GetMaterialXMaterialConfigs();
        if (logger_) {
            logger_->Trace("ShaderScriptService", "LoadShaderPathsMap",
                           "materialOverrides=" + std::to_string(materialOverrides.size()) +
                               ", baseEnabled=" + std::string(materialConfig.enabled ? "true" : "false"));
        }

        const auto scriptDirectory = engineService_
            ? engineService_->GetScriptDirectory()
            : std::filesystem::path{};

        const auto addShader = [&](const MaterialXConfig& config, const std::string& sourceLabel) {
            if (!config.enabled) {
                if (logger_) {
                    logger_->Trace("ShaderScriptService", "LoadShaderPathsMap",
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
                    logger_->Trace("ShaderScriptService", "LoadShaderPathsMap",
                                   "Skipping MaterialX shader generation (no document/constant color) key=" +
                                       config.shaderKey + ", source=" + sourceLabel);
                }
                return;
            }
            if (shaderMap.find(config.shaderKey) != shaderMap.end()) {
                if (logger_) {
                    logger_->Trace("ShaderScriptService", "LoadShaderPathsMap",
                                   "Skipping MaterialX shader generation (duplicate key) key=" +
                                       config.shaderKey + ", source=" + sourceLabel);
                }
                return;
            }
            if (logger_) {
                logger_->Trace("ShaderScriptService", "LoadShaderPathsMap",
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
    }

    if (shaderMap.empty()) {
        if (logger_) {
            logger_->Error("No MaterialX shaders were generated from JSON config");
        }
        throw std::runtime_error("No MaterialX shaders were generated from JSON config");
    }

    return shaderMap;
}

lua_State* ShaderScriptService::GetLuaState() const {
    if (logger_) {
        logger_->Trace("ShaderScriptService", "GetLuaState");
    }
    if (!engineService_) {
        throw std::runtime_error("Shader script service is missing script engine service");
    }
    lua_State* state = engineService_->GetLuaState();
    if (!state) {
        throw std::runtime_error("Lua state is not initialized");
    }
    return state;
}

}  // namespace sdl3cpp::services::impl
