#pragma once

#include "../interfaces/i_shader_script_service.hpp"
#include "../interfaces/i_script_engine_service.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Script-facing shader service implementation.
 */
class ShaderScriptService : public IShaderScriptService {
public:
    explicit ShaderScriptService(std::shared_ptr<IScriptEngineService> engineService);

    std::unordered_map<std::string, ShaderPaths> LoadShaderPathsMap() override;

private:
    std::shared_ptr<IScriptEngineService> engineService_;
};

}  // namespace sdl3cpp::services::impl
