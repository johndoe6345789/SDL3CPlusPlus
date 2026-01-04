#include "script/script_engine.hpp"
#include "script/scene_manager.hpp"
#include "script/shader_manager.hpp"
#include "script/gui_manager.hpp"
#include "script/audio_manager.hpp"
#include "script/lua_bindings.hpp"
#include "app/audio_player.hpp"
#include "logging/logger.hpp"

#include <lua.hpp>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::script {

ScriptEngine::ScriptEngine(const std::filesystem::path& scriptPath, bool debugEnabled)
    : L_(luaL_newstate()),
      scriptDirectory_(scriptPath.parent_path()),
      debugEnabled_(debugEnabled),
      physicsBridge_(std::make_unique<PhysicsBridge>()),
      sceneManager_(std::make_unique<SceneManager>(L_)),
      shaderManager_(std::make_unique<ShaderManager>(L_)),
      guiManager_(std::make_unique<GuiManager>(L_)),
      audioManager_(std::make_unique<AudioManager>(scriptDirectory_)) {
    sdl3cpp::logging::TraceGuard trace;;
    if (!L_) {
        sdl3cpp::logging::Logger::GetInstance().Error("Failed to create Lua state");
        throw std::runtime_error("Failed to create Lua state");
    }
    
    sdl3cpp::logging::Logger::GetInstance().Debug("Lua state created successfully");
    luaL_openlibs(L_);
    
    LuaBindings::RegisterBindings(L_, this);
    
    lua_pushboolean(L_, debugEnabled_);
    lua_setglobal(L_, "lua_debug");
    
    auto scriptDir = scriptPath.parent_path();
    if (!scriptDir.empty()) {
        lua_getglobal(L_, "package");
        if (lua_istable(L_, -1)) {
            lua_getfield(L_, -1, "path");
            const char* currentPath = lua_tostring(L_, -1);
            std::string newPath = scriptDir.string() + "/?.lua;";
            if (currentPath) {
                newPath += currentPath;
            }
            lua_pop(L_, 1);
            lua_pushstring(L_, newPath.c_str());
            lua_setfield(L_, -2, "path");
        }
        lua_pop(L_, 1);
    }
    
    if (luaL_dofile(L_, scriptPath.string().c_str()) != LUA_OK) {
        std::string message = sdl3cpp::script::GetLuaError(L_);
        lua_pop(L_, 1);
        lua_close(L_);
        L_ = nullptr;
        sdl3cpp::logging::Logger::GetInstance().Error("Failed to load Lua script: " + message);
        throw std::runtime_error("Failed to load Lua script: " + message);
    }

    sdl3cpp::logging::Logger::GetInstance().Info("Lua script loaded successfully: " + scriptPath.string());
}

ScriptEngine::~ScriptEngine() {
    if (L_) {
        lua_close(L_);
    }
}

std::vector<SceneManager::SceneObject> ScriptEngine::LoadSceneObjects() {
    return sceneManager_->LoadSceneObjects();
}

std::array<float, 16> ScriptEngine::ComputeModelMatrix(int functionRef, float time) {
    return sceneManager_->ComputeModelMatrix(functionRef, time);
}

std::array<float, 16> ScriptEngine::GetViewProjectionMatrix(float aspect) {
    return sceneManager_->GetViewProjectionMatrix(aspect);
}

std::unordered_map<std::string, ShaderManager::ShaderPaths> ScriptEngine::LoadShaderPathsMap() {
    return shaderManager_->LoadShaderPathsMap();
}

PhysicsBridge& ScriptEngine::GetPhysicsBridge() {
    return *physicsBridge_;
}

std::vector<GuiCommand> ScriptEngine::LoadGuiCommands() {
    return guiManager_->LoadGuiCommands();
}

void ScriptEngine::UpdateGuiInput(const GuiInputSnapshot& input) {
    guiManager_->UpdateGuiInput(input);
}

bool ScriptEngine::HasGuiCommands() const {
    return guiManager_->HasGuiCommands();
}

std::filesystem::path ScriptEngine::GetScriptDirectory() const {
    return scriptDirectory_;
}

void ScriptEngine::SetAudioPlayer(app::AudioPlayer* audioPlayer) {
    audioManager_->SetAudioPlayer(audioPlayer);
}

bool ScriptEngine::QueueAudioCommand(AudioManager::AudioCommandType type, std::string path, bool loop, std::string& error) {
    return audioManager_->QueueAudioCommand(type, path, loop, error);
}

std::string ScriptEngine::GetLuaError() {
    const char* message = lua_tostring(L_, -1);
    return message ? message : "unknown lua error";
}

} // namespace sdl3cpp::script