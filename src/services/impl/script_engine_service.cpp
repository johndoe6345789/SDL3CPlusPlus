#include "script_engine_service.hpp"

#include "lua_helpers.hpp"
#include "services/interfaces/i_logger.hpp"

#include <btBulletDynamicsCommon.h>
#include <lua.hpp>

#include <array>
#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

ScriptEngineService::ScriptEngineService(const std::filesystem::path& scriptPath,
                                         std::shared_ptr<ILogger> logger,
                                         std::shared_ptr<IMeshService> meshService,
                                         std::shared_ptr<IAudioCommandService> audioCommandService,
                                         std::shared_ptr<IPhysicsBridgeService> physicsBridgeService,
                                         bool debugEnabled)
    : logger_(std::move(logger)),
      meshService_(std::move(meshService)),
      audioCommandService_(std::move(audioCommandService)),
      physicsBridgeService_(std::move(physicsBridgeService)),
      scriptPath_(scriptPath),
      debugEnabled_(debugEnabled) {
}

ScriptEngineService::~ScriptEngineService() {
    if (initialized_) {
        Shutdown();
    }
}

void ScriptEngineService::Initialize() {
    if (initialized_) {
        return;
    }
    logger_->TraceFunction(__func__);

    bindingContext_ = std::make_shared<LuaBindingContext>();
    bindingContext_->meshService = meshService_;
    bindingContext_->audioCommandService = audioCommandService_;
    bindingContext_->physicsBridgeService = physicsBridgeService_;
    bindingContext_->logger = logger_;

    luaState_ = luaL_newstate();
    if (!luaState_) {
        logger_->Error("Failed to create Lua state");
        throw std::runtime_error("Failed to create Lua state");
    }

    luaL_openlibs(luaState_);
    RegisterBindings(luaState_);

    lua_pushboolean(luaState_, debugEnabled_);
    lua_setglobal(luaState_, "lua_debug");

    scriptDirectory_ = scriptPath_.parent_path();
    if (!scriptDirectory_.empty()) {
        lua_getglobal(luaState_, "package");
        if (lua_istable(luaState_, -1)) {
            lua_getfield(luaState_, -1, "path");
            const char* currentPath = lua_tostring(luaState_, -1);
            std::string newPath = scriptDirectory_.string() + "/?.lua;";
            if (currentPath) {
                newPath += currentPath;
            }
            lua_pop(luaState_, 1);
            lua_pushstring(luaState_, newPath.c_str());
            lua_setfield(luaState_, -2, "path");
        }
        lua_pop(luaState_, 1);
    }

    if (luaL_dofile(luaState_, scriptPath_.string().c_str()) != LUA_OK) {
        std::string message = lua::GetLuaError(luaState_);
        lua_pop(luaState_, 1);
        lua_close(luaState_);
        luaState_ = nullptr;
        if (logger_) {
            logger_->Error("Failed to load Lua script: " + message);
        }
        throw std::runtime_error("Failed to load Lua script: " + message);
    }

    initialized_ = true;
    logger_->Info("Script engine service initialized");
}

void ScriptEngineService::Shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    logger_->TraceFunction(__func__);

    if (luaState_) {
        lua_close(luaState_);
        luaState_ = nullptr;
    }
    bindingContext_.reset();
    initialized_ = false;

    logger_->Info("Script engine service shutdown");
}

lua_State* ScriptEngineService::GetLuaState() const {
    if (!luaState_) {
        throw std::runtime_error("Script engine service not initialized");
    }
    return luaState_;
}

std::filesystem::path ScriptEngineService::GetScriptDirectory() const {
    if (!luaState_) {
        throw std::runtime_error("Script engine service not initialized");
    }
    return scriptDirectory_;
}

void ScriptEngineService::RegisterBindings(lua_State* L) {
    if (logger_) {
        logger_->Trace("ScriptEngineService", "RegisterBindings");
    }
    if (!bindingContext_) {
        throw std::runtime_error("Lua binding context not initialized");
    }

    auto bind = [&](const char* name, lua_CFunction fn) {
        lua_pushlightuserdata(L, bindingContext_.get());
        lua_pushcclosure(L, fn, 1);
        lua_setglobal(L, name);
    };

    bind("load_mesh_from_file", &ScriptEngineService::LoadMeshFromFile);
    bind("physics_create_box", &ScriptEngineService::PhysicsCreateBox);
    bind("physics_step_simulation", &ScriptEngineService::PhysicsStepSimulation);
    bind("physics_get_transform", &ScriptEngineService::PhysicsGetTransform);
    bind("glm_matrix_from_transform", &ScriptEngineService::GlmMatrixFromTransform);
    bind("audio_play_background", &ScriptEngineService::AudioPlayBackground);
    bind("audio_play_sound", &ScriptEngineService::AudioPlaySound);
}

int ScriptEngineService::LoadMeshFromFile(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->meshService) {
        lua_pushnil(L);
        lua_pushstring(L, "Mesh service not available");
        return 2;
    }

    const char* path = luaL_checkstring(L, 1);
    if (logger) {
        logger->Trace("LuaBindings", "LoadMeshFromFile");
        logger->TraceVariable("path", std::string(path));
    }

    MeshPayload payload;
    std::string error;
    if (!context->meshService->LoadFromFile(path, payload, error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }

    context->meshService->PushMeshToLua(L, payload);
    lua_pushnil(L);
    return 2;
}

int ScriptEngineService::PhysicsCreateBox(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->physicsBridgeService) {
        lua_pushnil(L);
        lua_pushstring(L, "Physics service not available");
        return 2;
    }

    const char* name = luaL_checkstring(L, 1);
    if (logger) {
        logger->Trace("LuaBindings", "PhysicsCreateBox");
        logger->TraceVariable("name", std::string(name));
    }

    if (!lua_istable(L, 2) || !lua_istable(L, 4) || !lua_istable(L, 5)) {
        luaL_error(L, "physics_create_box expects vector tables for half extents, origin, and rotation");
    }

    std::array<float, 3> halfExtents = lua::ReadVector3(L, 2);
    float mass = static_cast<float>(luaL_checknumber(L, 3));
    std::array<float, 3> origin = lua::ReadVector3(L, 4);
    std::array<float, 4> rotation = lua::ReadQuaternion(L, 5);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(origin[0], origin[1], origin[2]));
    transform.setRotation(btQuaternion(rotation[0], rotation[1], rotation[2], rotation[3]));

    std::string error;
    if (!context->physicsBridgeService->AddBoxRigidBody(
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

int ScriptEngineService::PhysicsStepSimulation(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->physicsBridgeService) {
        lua_pushinteger(L, 0);
        return 1;
    }
    if (logger) {
        logger->Trace("LuaBindings", "PhysicsStepSimulation");
    }
    float deltaTime = static_cast<float>(luaL_checknumber(L, 1));
    int steps = context->physicsBridgeService->StepSimulation(deltaTime);
    lua_pushinteger(L, steps);
    return 1;
}

int ScriptEngineService::PhysicsGetTransform(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->physicsBridgeService) {
        lua_pushnil(L);
        lua_pushstring(L, "Physics service not available");
        return 2;
    }
    const char* name = luaL_checkstring(L, 1);
    if (logger) {
        logger->Trace("LuaBindings", "PhysicsGetTransform");
        logger->TraceVariable("name", std::string(name));
    }

    btTransform transform;
    std::string error;
    if (!context->physicsBridgeService->GetRigidBodyTransform(name, transform, error)) {
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

int ScriptEngineService::AudioPlayBackground(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->audioCommandService) {
        lua_pushnil(L);
        lua_pushstring(L, "Audio service not available");
        return 2;
    }
    const char* path = luaL_checkstring(L, 1);
    bool loop = true;
    if (lua_gettop(L) >= 2 && lua_isboolean(L, 2)) {
        loop = lua_toboolean(L, 2);
    }
    if (logger) {
        logger->Trace("LuaBindings", "AudioPlayBackground");
        logger->TraceVariable("path", std::string(path));
        logger->TraceVariable("loop", loop);
    }

    std::string error;
    if (!context->audioCommandService->QueueAudioCommand(
            AudioCommandType::Background, path, loop, error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

int ScriptEngineService::AudioPlaySound(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->audioCommandService) {
        lua_pushnil(L);
        lua_pushstring(L, "Audio service not available");
        return 2;
    }
    const char* path = luaL_checkstring(L, 1);
    bool loop = false;
    if (lua_gettop(L) >= 2 && lua_isboolean(L, 2)) {
        loop = lua_toboolean(L, 2);
    }
    if (logger) {
        logger->Trace("LuaBindings", "AudioPlaySound");
        logger->TraceVariable("path", std::string(path));
        logger->TraceVariable("loop", loop);
    }

    std::string error;
    if (!context->audioCommandService->QueueAudioCommand(
            AudioCommandType::Effect, path, loop, error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

int ScriptEngineService::GlmMatrixFromTransform(lua_State* L) {
    return lua::LuaGlmMatrixFromTransform(L);
}

}  // namespace sdl3cpp::services::impl
