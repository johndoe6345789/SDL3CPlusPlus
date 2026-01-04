#include "script/lua_bindings.hpp"
#include "script/script_engine.hpp"
#include "script/lua_helpers.hpp"
#include "script/mesh_loader.hpp"
#include "logging/logger.hpp"

#include <btBulletDynamicsCommon.h>
#include <lua.hpp>

namespace sdl3cpp::script {

void LuaBindings::RegisterBindings(lua_State* L, ScriptEngine* engine) {
    sdl3cpp::logging::TraceGuard trace(__PRETTY_FUNCTION__);;
    lua_pushlightuserdata(L, engine);
    lua_pushcclosure(L, &LoadMeshFromFile, 1);
    lua_setglobal(L, "load_mesh_from_file");

    lua_pushlightuserdata(L, engine);
    lua_pushcclosure(L, &PhysicsCreateBox, 1);
    lua_setglobal(L, "physics_create_box");

    lua_pushlightuserdata(L, engine);
    lua_pushcclosure(L, &PhysicsStepSimulation, 1);
    lua_setglobal(L, "physics_step_simulation");

    lua_pushlightuserdata(L, engine);
    lua_pushcclosure(L, &PhysicsGetTransform, 1);
    lua_setglobal(L, "physics_get_transform");

    lua_pushlightuserdata(L, engine);
    lua_pushcclosure(L, &GlmMatrixFromTransform, 1);
    lua_setglobal(L, "glm_matrix_from_transform");

    lua_pushlightuserdata(L, engine);
    lua_pushcclosure(L, &AudioPlayBackground, 1);
    lua_setglobal(L, "audio_play_background");

    lua_pushlightuserdata(L, engine);
    lua_pushcclosure(L, &AudioPlaySound, 1);
    lua_setglobal(L, "audio_play_sound");
}

int LuaBindings::LoadMeshFromFile(lua_State* L) {
    sdl3cpp::logging::TraceGuard trace(__PRETTY_FUNCTION__);;
    auto* engine = static_cast<ScriptEngine*>(lua_touserdata(L, lua_upvalueindex(1)));
    const char* path = luaL_checkstring(L, 1);
    sdl3cpp::logging::Logger::GetInstance().TraceVariable("path", path);
    
    MeshPayload payload;
    std::string error;
    if (!MeshLoader::LoadFromFile(engine->GetScriptDirectory(), path, payload, error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }
    
    MeshLoader::PushMeshToLua(L, payload);
    lua_pushnil(L);
    return 2;
}

int LuaBindings::PhysicsCreateBox(lua_State* L) {
    sdl3cpp::logging::TraceGuard trace(__PRETTY_FUNCTION__);;
    auto* engine = static_cast<ScriptEngine*>(lua_touserdata(L, lua_upvalueindex(1)));
    const char* name = luaL_checkstring(L, 1);
    sdl3cpp::logging::Logger::GetInstance().TraceVariable("name", name);
    
    if (!lua_istable(L, 2) || !lua_istable(L, 4) || !lua_istable(L, 5)) {
        luaL_error(L, "physics_create_box expects vector tables for half extents, origin, and rotation");
    }
    
    std::array<float, 3> halfExtents = ReadVector3(L, 2);
    float mass = static_cast<float>(luaL_checknumber(L, 3));
    std::array<float, 3> origin = ReadVector3(L, 4);
    std::array<float, 4> rotation = ReadQuaternion(L, 5);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(origin[0], origin[1], origin[2]));
    transform.setRotation(btQuaternion(rotation[0], rotation[1], rotation[2], rotation[3]));

    std::string error;
    if (!engine->GetPhysicsBridge().addBoxRigidBody(
            name,
            btVector3(halfExtents[0], halfExtents[1], halfExtents[2]),
            mass,
            transform,
            error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

int LuaBindings::PhysicsStepSimulation(lua_State* L) {
    auto* engine = static_cast<ScriptEngine*>(lua_touserdata(L, lua_upvalueindex(1)));
    float deltaTime = static_cast<float>(luaL_checknumber(L, 1));
    int steps = engine->GetPhysicsBridge().stepSimulation(deltaTime);
    lua_pushinteger(L, steps);
    return 1;
}

int LuaBindings::PhysicsGetTransform(lua_State* L) {
    auto* engine = static_cast<ScriptEngine*>(lua_touserdata(L, lua_upvalueindex(1)));
    const char* name = luaL_checkstring(L, 1);
    
    btTransform transform;
    std::string error;
    if (!engine->GetPhysicsBridge().getRigidBodyTransform(name, transform, error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }

    lua_newtable(L);
    lua_newtable(L);
    const btVector3& origin = transform.getOrigin();
    lua_pushnumber(L, origin.x());
    lua_rawseti(L, -2, 1);
    lua_pushnumber(L, origin.y());
    lua_rawseti(L, -2, 2);
    lua_pushnumber(L, origin.z());
    lua_rawseti(L, -2, 3);
    lua_setfield(L, -2, "position");

    lua_newtable(L);
    const btQuaternion& orientation = transform.getRotation();
    lua_pushnumber(L, orientation.x());
    lua_rawseti(L, -2, 1);
    lua_pushnumber(L, orientation.y());
    lua_rawseti(L, -2, 2);
    lua_pushnumber(L, orientation.z());
    lua_rawseti(L, -2, 3);
    lua_pushnumber(L, orientation.w());
    lua_rawseti(L, -2, 4);
    lua_setfield(L, -2, "rotation");

    return 1;
}

int LuaBindings::AudioPlayBackground(lua_State* L) {
    auto* engine = static_cast<ScriptEngine*>(lua_touserdata(L, lua_upvalueindex(1)));
    const char* path = luaL_checkstring(L, 1);
    bool loop = true;
    if (lua_gettop(L) >= 2 && lua_isboolean(L, 2)) {
        loop = lua_toboolean(L, 2);
    }
    
    std::string error;
    if (!engine->QueueAudioCommand(AudioManager::AudioCommandType::Background, path, loop, error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

int LuaBindings::AudioPlaySound(lua_State* L) {
    auto* engine = static_cast<ScriptEngine*>(lua_touserdata(L, lua_upvalueindex(1)));
    const char* path = luaL_checkstring(L, 1);
    bool loop = false;
    if (lua_gettop(L) >= 2 && lua_isboolean(L, 2)) {
        loop = lua_toboolean(L, 2);
    }
    
    std::string error;
    if (!engine->QueueAudioCommand(AudioManager::AudioCommandType::Effect, path, loop, error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

int LuaBindings::GlmMatrixFromTransform(lua_State* L) {
    return LuaGlmMatrixFromTransform(L);
}

} // namespace sdl3cpp::script
