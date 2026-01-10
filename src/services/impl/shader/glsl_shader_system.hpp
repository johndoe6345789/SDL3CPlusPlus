#pragma once

#include "services/interfaces/i_config_compiler_service.hpp"
#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_shader_system.hpp"

#include <memory>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/**
 * @brief GLSL shader system implementation backed by asset shader entries.
 */
class GlslShaderSystem final : public IShaderSystem {
public:
    GlslShaderSystem(std::shared_ptr<IConfigService> configService,
                     std::shared_ptr<IConfigCompilerService> configCompilerService,
                     std::shared_ptr<ILogger> logger);

    std::string GetId() const override { return "glsl"; }

    std::unordered_map<std::string, ShaderPaths> BuildShaderMap() override;

    ShaderReflection GetReflection(const std::string& shaderKey) const override;

    std::vector<ShaderPaths::TextureBinding> GetDefaultTextures(const std::string& shaderKey) const override;

private:
    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<IConfigCompilerService> configCompilerService_;
    std::shared_ptr<ILogger> logger_;
    std::unordered_map<std::string, ShaderPaths> lastShaderMap_;
};

}  // namespace sdl3cpp::services::impl
