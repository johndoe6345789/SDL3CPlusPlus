#include "script/script_engine.hpp"
#include "script/scene_manager.hpp"
#include "script/shader_manager.hpp"
#include "script/gui_manager.hpp"
#include "script/lua_bindings.hpp"
#include "services/interfaces/i_logger.hpp"

#include <lua.hpp>

#include <stdexcept>

namespace sdl3cpp::script {

ScriptEngine::ScriptEngine(const std::filesystem::path& scriptPath, bool debugEnabled)
    : ScriptEngine(scriptPath, nullptr, debugEnabled) {
}

ScriptEngine::ScriptEngine(const std::filesystem::path& scriptPath, LuaBindingContext* bindingContext, bool debugEnabled)
    : L_(luaL_newstate()),
      scriptDirectory_(scriptPath.parent_path()),
      debugEnabled_(debugEnabled) {
    LuaBindingContext* resolvedContext = bindingContext;
    if (!resolvedContext) {
        ownedBindingContext_ = std::make_unique<LuaBindingContext>();
        resolvedContext = ownedBindingContext_.get();
    }
    if (resolvedContext) {
        logger_ = resolvedContext->logger;
    }
    if (logger_) {
        logger_->Trace("ScriptEngine", "ScriptEngine");
    }
    if (!L_) {
        if (logger_) {
            logger_->Error("Failed to create Lua state");
        }
        throw std::runtime_error("Failed to create Lua state");
    }

    if (logger_) {
        logger_->Debug("Lua state created successfully");
    }
    luaL_openlibs(L_);

    LuaBindings::RegisterBindings(L_, resolvedContext);

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
        if (logger_) {
            logger_->Error("Failed to load Lua script: " + message);
        }
        throw std::runtime_error("Failed to load Lua script: " + message);
    }

    if (logger_) {
        logger_->Info("Lua script loaded successfully: " + scriptPath.string());
    }

    sceneManager_ = std::make_unique<SceneManager>(L_, logger_);
    shaderManager_ = std::make_unique<ShaderManager>(L_, logger_);
    guiManager_ = std::make_unique<GuiManager>(L_, logger_);
}

ScriptEngine::~ScriptEngine() {
    if (L_) {
        lua_close(L_);
    }
}

std::vector<SceneManager::SceneObject> ScriptEngine::LoadSceneObjects() {
    if (!sceneManager_) {
        throw std::runtime_error("Scene manager not initialized");
    }
    return sceneManager_->LoadSceneObjects();
}

std::array<float, 16> ScriptEngine::ComputeModelMatrix(int functionRef, float time) {
    if (!sceneManager_) {
        throw std::runtime_error("Scene manager not initialized");
    }
    return sceneManager_->ComputeModelMatrix(functionRef, time);
}

std::array<float, 16> ScriptEngine::GetViewProjectionMatrix(float aspect) {
    if (!sceneManager_) {
        throw std::runtime_error("Scene manager not initialized");
    }
    return sceneManager_->GetViewProjectionMatrix(aspect);
}

std::unordered_map<std::string, sdl3cpp::services::ShaderPaths> ScriptEngine::LoadShaderPathsMap() {
    if (!shaderManager_) {
        throw std::runtime_error("Shader manager not initialized");
    }
    return shaderManager_->LoadShaderPathsMap();
}

std::vector<GuiCommand> ScriptEngine::LoadGuiCommands() {
    if (!guiManager_) {
        throw std::runtime_error("Gui manager not initialized");
    }
    return guiManager_->LoadGuiCommands();
}

void ScriptEngine::UpdateGuiInput(const GuiInputSnapshot& input) {
    if (!guiManager_) {
        throw std::runtime_error("Gui manager not initialized");
    }
    guiManager_->UpdateGuiInput(input);
}

bool ScriptEngine::HasGuiCommands() const {
    if (!guiManager_) {
        return false;
    }
    return guiManager_->HasGuiCommands();
}

std::filesystem::path ScriptEngine::GetScriptDirectory() const {
    return scriptDirectory_;
}

std::string ScriptEngine::GetLuaError() {
    const char* message = lua_tostring(L_, -1);
    return message ? message : "unknown lua error";
}

} // namespace sdl3cpp::script
