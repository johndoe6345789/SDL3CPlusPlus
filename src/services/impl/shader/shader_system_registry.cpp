#include "shader_system_registry.hpp"

#include <rapidjson/document.h>

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

ShaderSystemRegistry::ShaderSystemRegistry(std::shared_ptr<IConfigService> configService,
                                           std::shared_ptr<IConfigCompilerService> configCompilerService,
                                           std::shared_ptr<ILogger> logger)
    : configService_(std::move(configService)),
      logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("ShaderSystemRegistry", "ShaderSystemRegistry",
                       "configService=" + std::string(configService_ ? "set" : "null") +
                           ", configCompilerService=" + std::string(configCompilerService ? "set" : "null"));
    }

    if (configCompilerService) {
        auto glslSystem = std::make_shared<GlslShaderSystem>(configService_, configCompilerService, logger_);
        systems_.emplace(glslSystem->GetId(), std::move(glslSystem));
    } else if (logger_) {
        logger_->Warn("ShaderSystemRegistry: Config compiler missing; GLSL shader system disabled");
    }

    auto materialxSystem = std::make_shared<MaterialXShaderSystem>(configService_, logger_);
    systems_.emplace(materialxSystem->GetId(), std::move(materialxSystem));
}

std::unordered_map<std::string, ShaderPaths> ShaderSystemRegistry::BuildShaderMap() {
    const std::string activeSystem = ResolveActiveSystemId();
    auto it = systems_.find(activeSystem);
    if (it == systems_.end()) {
        throw std::runtime_error("Active shader system not registered: " + activeSystem);
    }

    if (logger_) {
        logger_->Trace("ShaderSystemRegistry", "BuildShaderMap", "activeSystem=" + activeSystem);
    }

    return it->second->BuildShaderMap();
}

ShaderReflection ShaderSystemRegistry::GetReflection(const std::string& shaderKey) const {
    const std::string activeSystem = ResolveActiveSystemId();
    auto it = systems_.find(activeSystem);
    if (it == systems_.end()) {
        if (logger_) {
            logger_->Warn("ShaderSystemRegistry::GetReflection: Active system not registered");
        }
        return {};
    }
    return it->second->GetReflection(shaderKey);
}

std::vector<ShaderPaths::TextureBinding> ShaderSystemRegistry::GetDefaultTextures(
    const std::string& shaderKey) const {
    const std::string activeSystem = ResolveActiveSystemId();
    auto it = systems_.find(activeSystem);
    if (it == systems_.end()) {
        if (logger_) {
            logger_->Warn("ShaderSystemRegistry::GetDefaultTextures: Active system not registered");
        }
        return {};
    }
    return it->second->GetDefaultTextures(shaderKey);
}

std::string ShaderSystemRegistry::GetActiveSystemId() const {
    return ResolveActiveSystemId();
}

std::string ShaderSystemRegistry::ResolveActiveSystemId() const {
    std::string activeSystem = "materialx";
    const std::string& configJson = configService_ ? configService_->GetConfigJson() : std::string{};
    if (!configJson.empty()) {
        rapidjson::Document document;
        document.Parse(configJson.c_str());
        if (document.IsObject() && document.HasMember("shader_systems")) {
            const auto& shaderSystemsValue = document["shader_systems"];
            if (shaderSystemsValue.IsObject() && shaderSystemsValue.HasMember("active")) {
                const auto& activeValue = shaderSystemsValue["active"];
                if (activeValue.IsString()) {
                    activeSystem = activeValue.GetString();
                } else if (logger_) {
                    logger_->Warn("ShaderSystemRegistry::ResolveActiveSystemId: shader_systems.active must be a string");
                }
            }
        }
    }

    if (!IsSystemEnabled(activeSystem)) {
        if (logger_) {
            logger_->Warn("ShaderSystemRegistry::ResolveActiveSystemId: active system disabled, falling back");
        }
        activeSystem = ResolveFallbackSystemId();
    }

    if (systems_.find(activeSystem) == systems_.end()) {
        if (logger_) {
            logger_->Warn("ShaderSystemRegistry::ResolveActiveSystemId: active system not registered, falling back");
        }
        activeSystem = ResolveFallbackSystemId();
    }

    if (activeSystem.empty()) {
        return {};
    }

    return activeSystem;
}

std::string ShaderSystemRegistry::ResolveFallbackSystemId() const {
    auto it = systems_.find("materialx");
    if (it != systems_.end()) {
        return it->first;
    }
    if (!systems_.empty()) {
        return systems_.begin()->first;
    }
    return {};
}

bool ShaderSystemRegistry::IsSystemEnabled(const std::string& systemId) const {
    if (systemId.empty()) {
        return false;
    }
    const std::string& configJson = configService_ ? configService_->GetConfigJson() : std::string{};
    if (configJson.empty()) {
        return true;
    }
    rapidjson::Document document;
    document.Parse(configJson.c_str());
    if (!document.IsObject() || !document.HasMember("shader_systems")) {
        return true;
    }
    const auto& shaderSystemsValue = document["shader_systems"];
    if (!shaderSystemsValue.IsObject() || !shaderSystemsValue.HasMember("systems")) {
        return true;
    }
    const auto& systemsValue = shaderSystemsValue["systems"];
    if (!systemsValue.IsObject() || !systemsValue.HasMember(systemId.c_str())) {
        return true;
    }
    const auto& systemValue = systemsValue[systemId.c_str()];
    if (!systemValue.IsObject()) {
        return true;
    }
    if (!systemValue.HasMember("enabled")) {
        return true;
    }
    const auto& enabledValue = systemValue["enabled"];
    if (!enabledValue.IsBool()) {
        if (logger_) {
            logger_->Warn("ShaderSystemRegistry::IsSystemEnabled: enabled must be boolean for system=" + systemId);
        }
        return true;
    }
    return enabledValue.GetBool();
}

}  // namespace sdl3cpp::services::impl
