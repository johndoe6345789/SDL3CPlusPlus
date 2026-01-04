#ifndef SDL3CPP_SCRIPT_SCRIPT_ENGINE_HPP
#define SDL3CPP_SCRIPT_SCRIPT_ENGINE_HPP

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <lua.hpp>

#include "script/gui_types.hpp"
#include "script/scene_manager.hpp"
#include "script/shader_manager.hpp"
#include "script/gui_manager.hpp"

namespace sdl3cpp::script {

struct LuaBindingContext;

}

namespace sdl3cpp::services {
class ILogger;
}

namespace sdl3cpp::script {

class ScriptEngine {
public:
    explicit ScriptEngine(const std::filesystem::path& scriptPath, bool debugEnabled = false);
    ScriptEngine(const std::filesystem::path& scriptPath, LuaBindingContext* bindingContext, bool debugEnabled = false);
    ~ScriptEngine();

    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

    std::vector<SceneManager::SceneObject> LoadSceneObjects();
    std::array<float, 16> ComputeModelMatrix(int functionRef, float time);
    std::array<float, 16> GetViewProjectionMatrix(float aspect);
    std::unordered_map<std::string, sdl3cpp::services::ShaderPaths> LoadShaderPathsMap();
    std::vector<GuiCommand> LoadGuiCommands();
    void UpdateGuiInput(const GuiInputSnapshot& input);
    bool HasGuiCommands() const;
    std::filesystem::path GetScriptDirectory() const;
    std::string GetLuaError();

private:
    lua_State* L_ = nullptr;
    std::filesystem::path scriptDirectory_;
    bool debugEnabled_ = false;
    std::unique_ptr<LuaBindingContext> ownedBindingContext_;
    std::unique_ptr<SceneManager> sceneManager_;
    std::unique_ptr<ShaderManager> shaderManager_;
    std::unique_ptr<GuiManager> guiManager_;
    std::shared_ptr<services::ILogger> logger_;
};

} // namespace sdl3cpp::script

#endif // SDL3CPP_SCRIPT_SCRIPT_ENGINE_HPP
