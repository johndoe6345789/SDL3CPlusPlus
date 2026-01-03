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

    struct ShaderPaths {
        std::string vertex;
        std::string fragment;
    };

    struct SceneObject {
        std::vector<core::Vertex> vertices;
        std::vector<uint16_t> indices;
        int computeModelMatrixRef = LUA_REFNIL;
        std::string shaderKey = "default";
    };

    enum class AudioCommandType {
        Background,
        Effect,
    };

    std::vector<SceneObject> LoadSceneObjects();
    std::array<float, 16> ComputeModelMatrix(int functionRef, float time);
    std::array<float, 16> GetViewProjectionMatrix(float aspect);
    std::unordered_map<std::string, ShaderPaths> LoadShaderPathsMap();
    std::vector<GuiCommand> LoadGuiCommands();
    void UpdateGuiInput(const GuiInputSnapshot& input);
    bool HasGuiCommands() const;
    std::filesystem::path GetScriptDirectory() const;
    PhysicsBridge& GetPhysicsBridge();
    void SetAudioPlayer(app::AudioPlayer* audioPlayer);
    bool QueueAudioCommand(AudioCommandType type, std::string path, bool loop, std::string& error);

private:
    struct AudioCommand {
        AudioCommandType type = AudioCommandType::Background;
        std::string path;
        bool loop = false;
    };

    void ExecuteAudioCommand(app::AudioPlayer* player, const AudioCommand& command);
    std::filesystem::path ResolveScriptPath(const std::string& requested) const;

    static std::vector<core::Vertex> ReadVertexArray(lua_State* L, int index);
    static std::vector<uint16_t> ReadIndexArray(lua_State* L, int index);
    static std::string LuaErrorMessage(lua_State* L);
    static ShaderPaths ReadShaderPathsTable(lua_State* L, int index);
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
    std::vector<AudioCommand> pendingAudioCommands_;
};

} // namespace sdl3cpp::script

#endif // SDL3CPP_SCRIPT_SCRIPT_ENGINE_HPP
