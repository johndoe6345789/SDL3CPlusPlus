#ifndef SDL3CPP_SCRIPT_CUBE_SCRIPT_HPP
#define SDL3CPP_SCRIPT_CUBE_SCRIPT_HPP

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <lua.hpp>

#include "core/vertex.hpp"

namespace sdl3cpp::script {

struct PhysicsBridge;

struct GuiInputSnapshot {
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    bool mouseDown = false;
    float wheel = 0.0f;
    std::string textInput;
    std::unordered_map<std::string, bool> keyStates;
};

struct GuiColor {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 1.0f;
};

struct GuiCommand {
    enum class Type {
        Rect,
        Text,
        ClipPush,
        ClipPop,
        Svg,
    };

    struct RectData {
        float x = 0;
        float y = 0;
        float width = 0;
        float height = 0;
    };

    Type type = Type::Rect;
    RectData rect;
    GuiColor color;
    GuiColor borderColor;
    float borderWidth = 0.0f;
    bool hasClipRect = false;
    RectData clipRect{};
    std::string text;
    float fontSize = 16.0f;
    std::string alignX = "left";
    std::string alignY = "center";
    std::string svgPath;
    GuiColor svgTint;
    RectData bounds{};
    bool hasBounds = false;
};

class CubeScript {
public:
    using GuiCommand = ::sdl3cpp::script::GuiCommand;
    using GuiColor = ::sdl3cpp::script::GuiColor;

public:
    explicit CubeScript(const std::filesystem::path& scriptPath, bool debugEnabled = false);
    ~CubeScript();

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

    std::vector<SceneObject> LoadSceneObjects();
    std::array<float, 16> ComputeModelMatrix(int functionRef, float time);
    std::array<float, 16> GetViewProjectionMatrix(float aspect);
    std::unordered_map<std::string, ShaderPaths> LoadShaderPathsMap();
    std::vector<GuiCommand> LoadGuiCommands();
    void UpdateGuiInput(const GuiInputSnapshot& input);
    bool HasGuiCommands() const;
    std::filesystem::path GetScriptDirectory() const;
    PhysicsBridge& GetPhysicsBridge();

private:
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
};

} // namespace sdl3cpp::script

#endif // SDL3CPP_SCRIPT_CUBE_SCRIPT_HPP
