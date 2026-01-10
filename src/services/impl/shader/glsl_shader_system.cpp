#include "glsl_shader_system.hpp"

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

GlslShaderSystem::GlslShaderSystem(std::shared_ptr<IConfigService> configService,
                                   std::shared_ptr<IConfigCompilerService> configCompilerService,
                                   std::shared_ptr<ILogger> logger)
    : configService_(std::move(configService)),
      configCompilerService_(std::move(configCompilerService)),
      logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("GlslShaderSystem", "GlslShaderSystem",
                       "configService=" + std::string(configService_ ? "set" : "null") +
                           ", configCompilerService=" + std::string(configCompilerService_ ? "set" : "null"));
    }
}

std::unordered_map<std::string, ShaderPaths> GlslShaderSystem::BuildShaderMap() {
    if (logger_) {
        logger_->Trace("GlslShaderSystem", "BuildShaderMap", "source=assets.shaders");
    }

    if (!configService_ || !configCompilerService_) {
        throw std::runtime_error("GlslShaderSystem requires config and compiler services");
    }

    const std::string& configJson = configService_->GetConfigJson();
    if (configJson.empty()) {
        throw std::runtime_error("GlslShaderSystem requires JSON config content");
    }

    const auto compileResult = configCompilerService_->Compile(configJson);
    if (logger_) {
        logger_->Trace("GlslShaderSystem", "BuildShaderMap",
                       "shaderCount=" + std::to_string(compileResult.resources.shaders.size()) +
                           ", compileSuccess=" + std::string(compileResult.success ? "true" : "false"));
    }

    std::unordered_map<std::string, ShaderPaths> shaderMap;
    for (const auto& shader : compileResult.resources.shaders) {
        if (!shader.system.empty() && shader.system != GetId()) {
            continue;
        }
        ShaderPaths paths;
        paths.vertex = shader.vertexPath;
        paths.fragment = shader.fragmentPath;
        auto [it, inserted] = shaderMap.emplace(shader.id, std::move(paths));
        if (!inserted && logger_) {
            logger_->Warn("GlslShaderSystem::BuildShaderMap: Duplicate shader id skipped: " + shader.id);
        }
    }

    if (shaderMap.empty()) {
        if (logger_) {
            logger_->Error("GlslShaderSystem::BuildShaderMap: No GLSL shaders found in assets.shaders");
        }
        throw std::runtime_error("No GLSL shaders found in assets.shaders");
    }

    lastShaderMap_ = shaderMap;
    return shaderMap;
}

ShaderReflection GlslShaderSystem::GetReflection(const std::string& shaderKey) const {
    if (logger_) {
        logger_->Trace("GlslShaderSystem", "GetReflection", "shaderKey=" + shaderKey);
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

std::vector<ShaderPaths::TextureBinding> GlslShaderSystem::GetDefaultTextures(
    const std::string& shaderKey) const {
    if (logger_) {
        logger_->Trace("GlslShaderSystem", "GetDefaultTextures", "shaderKey=" + shaderKey);
    }
    auto it = lastShaderMap_.find(shaderKey);
    if (it == lastShaderMap_.end()) {
        return {};
    }
    return it->second.textures;
}

}  // namespace sdl3cpp::services::impl
