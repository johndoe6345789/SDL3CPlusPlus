#ifndef SDL3CPP_SCRIPT_LUA_BINDINGS_HPP
#define SDL3CPP_SCRIPT_LUA_BINDINGS_HPP

#include <memory>

struct lua_State;

namespace sdl3cpp::services {
class IAudioCommandService;
class IMeshService;
class IPhysicsBridgeService;
}

namespace sdl3cpp::script {

class ScriptEngine;

struct LuaBindingContext {
    std::shared_ptr<services::IMeshService> meshService;
    std::shared_ptr<services::IAudioCommandService> audioCommandService;
    std::shared_ptr<services::IPhysicsBridgeService> physicsBridgeService;
};

class LuaBindings {
public:
    static void RegisterBindings(lua_State* L, ScriptEngine* engine);
    static void RegisterBindings(lua_State* L, LuaBindingContext* context);

private:
    static int LoadMeshFromFile(lua_State* L);
    static int LoadMeshFromFileWithServices(lua_State* L);
    static int PhysicsCreateBox(lua_State* L);
    static int PhysicsCreateBoxWithServices(lua_State* L);
    static int PhysicsStepSimulation(lua_State* L);
    static int PhysicsStepSimulationWithServices(lua_State* L);
    static int PhysicsGetTransform(lua_State* L);
    static int PhysicsGetTransformWithServices(lua_State* L);
    static int AudioPlayBackground(lua_State* L);
    static int AudioPlayBackgroundWithServices(lua_State* L);
    static int AudioPlaySound(lua_State* L);
    static int AudioPlaySoundWithServices(lua_State* L);
    static int GlmMatrixFromTransform(lua_State* L);
};

} // namespace sdl3cpp::script

#endif // SDL3CPP_SCRIPT_LUA_BINDINGS_HPP
