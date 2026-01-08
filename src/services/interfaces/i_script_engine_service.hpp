#pragma once

#include <filesystem>

struct lua_State;

namespace sdl3cpp::services {

/**
 * @brief Service for owning and exposing the Lua script engine.
 */
class IScriptEngineService {
public:
    virtual ~IScriptEngineService() = default;

    virtual lua_State* GetLuaState() const = 0;
    virtual std::filesystem::path GetScriptDirectory() const = 0;
    virtual bool IsInitialized() const = 0;
};

}  // namespace sdl3cpp::services
