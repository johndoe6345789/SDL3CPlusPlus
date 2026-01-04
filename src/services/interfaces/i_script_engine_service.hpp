#pragma once

#include <filesystem>

namespace sdl3cpp::script {
class ScriptEngine;
}

namespace sdl3cpp::services {

/**
 * @brief Service for owning and exposing the Lua script engine.
 */
class IScriptEngineService {
public:
    virtual ~IScriptEngineService() = default;

    virtual script::ScriptEngine& GetEngine() = 0;
    virtual std::filesystem::path GetScriptDirectory() const = 0;
    virtual bool IsInitialized() const = 0;
};

}  // namespace sdl3cpp::services
