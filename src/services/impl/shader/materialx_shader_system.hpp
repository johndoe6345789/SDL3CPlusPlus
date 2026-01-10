#pragma once

#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_shader_system.hpp"
#include "services/impl/materialx/materialx_shader_generator.hpp"

#include <memory>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/**
 * @brief MaterialX-backed shader system implementation.
 */
class MaterialXShaderSystem final : public IShaderSystem {
public:
    MaterialXShaderSystem(std::shared_ptr<IConfigService> configService,
                          std::shared_ptr<ILogger> logger);

    std::string GetId() const override { return "materialx"; }

    std::unordered_map<std::string, ShaderPaths> BuildShaderMap() override;

    ShaderReflection GetReflection(const std::string& shaderKey) const override;

    std::vector<ShaderPaths::TextureBinding> GetDefaultTextures(const std::string& shaderKey) const override;

private:
    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<ILogger> logger_;
    MaterialXShaderGenerator materialxGenerator_;
    std::unordered_map<std::string, ShaderPaths> lastShaderMap_;
};

}  // namespace sdl3cpp::services::impl
