#pragma once

#include "../interfaces/i_shader_script_service.hpp"
#include "../interfaces/i_script_engine_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <memory>

struct lua_State;

namespace sdl3cpp::services::impl {

/**
 * @brief Script-facing shader service implementation.
 */
class ShaderScriptService : public IShaderScriptService {
public:
    ShaderScriptService(std::shared_ptr<IScriptEngineService> engineService,
                        std::shared_ptr<ILogger> logger);

    std::unordered_map<std::string, ShaderPaths> LoadShaderPathsMap() override;

private:
    lua_State* GetLuaState() const;
    ShaderPaths ReadShaderPathsTable(lua_State* L, int index) const;

    std::shared_ptr<IScriptEngineService> engineService_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
