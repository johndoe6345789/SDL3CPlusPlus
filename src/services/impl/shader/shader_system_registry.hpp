#pragma once

#include "services/interfaces/i_config_compiler_service.hpp"
#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_shader_system_registry.hpp"
#include "glsl_shader_system.hpp"
#include "materialx_shader_system.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/**
 * @brief Registry for shader systems with active system selection.
 */
class ShaderSystemRegistry final : public IShaderSystemRegistry {
public:
    ShaderSystemRegistry(std::shared_ptr<IConfigService> configService,
                         std::shared_ptr<IConfigCompilerService> configCompilerService,
                         std::shared_ptr<ILogger> logger);

    std::unordered_map<std::string, ShaderPaths> BuildShaderMap() override;

    ShaderReflection GetReflection(const std::string& shaderKey) const override;

    std::vector<ShaderPaths::TextureBinding> GetDefaultTextures(
        const std::string& shaderKey) const override;

    std::string GetActiveSystemId() const override;

private:
    std::string ResolveActiveSystemId() const;
    std::string ResolveFallbackSystemId() const;
    bool IsSystemEnabled(const std::string& systemId) const;

    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<ILogger> logger_;
    std::unordered_map<std::string, std::shared_ptr<IShaderSystem>> systems_;
};

}  // namespace sdl3cpp::services::impl
