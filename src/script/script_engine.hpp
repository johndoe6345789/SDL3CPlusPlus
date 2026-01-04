#ifndef SDL3CPP_SCRIPT_SCRIPT_ENGINE_HPP
#define SDL3CPP_SCRIPT_SCRIPT_ENGINE_HPP

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <lua.hpp>

#include "core/vertex.hpp"
#include "script/gui_types.hpp"
#include "script/physics_bridge.hpp"
#include "script/scene_manager.hpp"
#include "script/shader_manager.hpp"
#include "script/gui_manager.hpp"
#include "script/audio_manager.hpp"

namespace sdl3cpp::app {
class AudioPlayer;
}

namespace sdl3cpp::script {

class ScriptEngine {
public:
    explicit ScriptEngine(const std::filesystem::path& scriptPath, bool debugEnabled = false);
    ~ScriptEngine();

    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

    std::vector<SceneManager::SceneObject> LoadSceneObjects();
    std::array<float, 16> ComputeModelMatrix(int functionRef, float time);
    std::array<float, 16> GetViewProjectionMatrix(float aspect);
    std::unordered_map<std::string, ShaderManager::ShaderPaths> LoadShaderPathsMap();
    std::vector<GuiCommand> LoadGuiCommands();
    void UpdateGuiInput(const GuiInputSnapshot& input);
    bool HasGuiCommands() const;
    std::filesystem::path GetScriptDirectory() const;
    PhysicsBridge& GetPhysicsBridge();
    void SetAudioPlayer(app::AudioPlayer* audioPlayer);
    bool QueueAudioCommand(AudioManager::AudioCommandType type, std::string path, bool loop, std::string& error);
    std::string GetLuaError();

private:
private:
    void ExecuteAudioCommand(app::AudioPlayer* player, const AudioManager::AudioCommand& command);
    std::filesystem::path ResolveScriptPath(const std::string& requested) const;

    static std::vector<core::Vertex> ReadVertexArray(lua_State* L, int index);
    static std::vector<uint16_t> ReadIndexArray(lua_State* L, int index);
    static std::string LuaErrorMessage(lua_State* L);
    static GuiCommand::RectData ReadRect(lua_State* L, int index);
    static GuiColor ReadColor(lua_State* L, int index, const GuiColor& defaultColor);
    static bool ReadStringField(lua_State* L, int index, const char* name, std::string& outString);

    lua_State* L_ = nullptr;
    int guiInputRef_ = LUA_REFNIL;
    int guiCommandsFnRef_ = LUA_REFNIL;
    std::filesystem::path scriptDirectory_;
    bool debugEnabled_ = false;
    std::unique_ptr<PhysicsBridge> physicsBridge_;
    app::AudioPlayer* audioPlayer_ = nullptr;
    std::vector<AudioManager::AudioCommand> pendingAudioCommands_;
    std::unique_ptr<SceneManager> sceneManager_;
    std::unique_ptr<ShaderManager> shaderManager_;
    std::unique_ptr<GuiManager> guiManager_;
    std::unique_ptr<AudioManager> audioManager_;
};

} // namespace sdl3cpp::script

#endif // SDL3CPP_SCRIPT_SCRIPT_ENGINE_HPP
