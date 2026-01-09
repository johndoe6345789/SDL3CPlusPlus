#pragma once

#include "../interfaces/i_config_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../interfaces/i_script_engine_service.hpp"
#include "../interfaces/i_shader_system.hpp"
#include "materialx_shader_generator.hpp"

#include <memory>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/**
 * @brief MaterialX-backed shader system implementation.
 */
class MaterialXShaderSystem final : public IShaderSystem {
public:
    MaterialXShaderSystem(std::shared_ptr<IConfigService> configService,
                          std::shared_ptr<IScriptEngineService> scriptEngineService,
                          std::shared_ptr<ILogger> logger);

    std::string GetId() const override { return "materialx"; }

    std::unordered_map<std::string, ShaderPaths> BuildShaderMap() override;

    ShaderReflection GetReflection(const std::string& shaderKey) const override;

    std::vector<ShaderPaths::TextureBinding> GetDefaultTextures(const std::string& shaderKey) const override;

private:
    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<IScriptEngineService> scriptEngineService_;
    std::shared_ptr<ILogger> logger_;
    MaterialXShaderGenerator materialxGenerator_;
};

}  // namespace sdl3cpp::services::impl
