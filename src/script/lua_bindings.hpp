#ifndef SDL3CPP_SCRIPT_LUA_BINDINGS_HPP
#define SDL3CPP_SCRIPT_LUA_BINDINGS_HPP

struct lua_State;

namespace sdl3cpp::script {

class ScriptEngine;

class LuaBindings {
public:
    static void RegisterBindings(lua_State* L, ScriptEngine* engine);

private:
    static int LoadMeshFromFile(lua_State* L);
    static int PhysicsCreateBox(lua_State* L);
    static int PhysicsStepSimulation(lua_State* L);
    static int PhysicsGetTransform(lua_State* L);
    static int AudioPlayBackground(lua_State* L);
    static int AudioPlaySound(lua_State* L);
    static int GlmMatrixFromTransform(lua_State* L);
};

} // namespace sdl3cpp::script

#endif // SDL3CPP_SCRIPT_LUA_BINDINGS_HPP
