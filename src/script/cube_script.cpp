#include "script/cube_script.hpp"

#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::script {

namespace {

std::array<float, 16> IdentityMatrix() {
    return {1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f};
}

} // namespace

CubeScript::CubeScript(const std::filesystem::path& scriptPath)
    : L_(luaL_newstate()), scriptDirectory_(scriptPath.parent_path()) {
    if (!L_) {
        throw std::runtime_error("Failed to create Lua state");
    }
    luaL_openlibs(L_);
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
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        lua_close(L_);
        L_ = nullptr;
        throw std::runtime_error("Failed to load Lua script: " + message);
    }

    lua_getglobal(L_, "gui_input");
    if (!lua_isnil(L_, -1)) {
        guiInputRef_ = luaL_ref(L_, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L_, 1);
    }

    lua_getglobal(L_, "get_gui_commands");
    if (lua_isfunction(L_, -1)) {
        guiCommandsFnRef_ = luaL_ref(L_, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L_, 1);
    }
}

CubeScript::~CubeScript() {
    if (L_) {
        if (guiInputRef_ != LUA_REFNIL) {
            luaL_unref(L_, LUA_REGISTRYINDEX, guiInputRef_);
        }
        if (guiCommandsFnRef_ != LUA_REFNIL) {
            luaL_unref(L_, LUA_REGISTRYINDEX, guiCommandsFnRef_);
        }
        lua_close(L_);
    }
}

std::vector<CubeScript::SceneObject> CubeScript::LoadSceneObjects() {
    lua_getglobal(L_, "get_scene_objects");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("Lua function 'get_scene_objects' is missing");
    }
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        throw std::runtime_error("Lua get_scene_objects failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("'get_scene_objects' did not return a table");
    }

    size_t count = lua_rawlen(L_, -1);
    std::vector<SceneObject> objects;
    objects.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L_, -1, static_cast<int>(i));
        if (!lua_istable(L_, -1)) {
            lua_pop(L_, 1);
            throw std::runtime_error("Scene object at index " + std::to_string(i) + " is not a table");
        }

        SceneObject object;
        lua_getfield(L_, -1, "vertices");
        object.vertices = ReadVertexArray(L_, -1);
        lua_pop(L_, 1);
        if (object.vertices.empty()) {
            lua_pop(L_, 1);
            throw std::runtime_error("Scene object " + std::to_string(i) + " must supply at least one vertex");
        }

        lua_getfield(L_, -1, "indices");
        object.indices = ReadIndexArray(L_, -1);
        lua_pop(L_, 1);
        if (object.indices.empty()) {
            lua_pop(L_, 1);
            throw std::runtime_error("Scene object " + std::to_string(i) + " must supply indices");
        }

        lua_getfield(L_, -1, "compute_model_matrix");
        if (lua_isfunction(L_, -1)) {
            object.computeModelMatrixRef = luaL_ref(L_, LUA_REGISTRYINDEX);
        } else {
            lua_pop(L_, 1);
            object.computeModelMatrixRef = LUA_REFNIL;
        }

        lua_getfield(L_, -1, "shader_key");
        if (lua_isstring(L_, -1)) {
            object.shaderKey = lua_tostring(L_, -1);
        }
        lua_pop(L_, 1);

        objects.push_back(std::move(object));
        lua_pop(L_, 1);
    }

    lua_pop(L_, 1);
    return objects;
}

std::array<float, 16> CubeScript::ComputeModelMatrix(int functionRef, float time) {
    if (functionRef == LUA_REFNIL) {
        lua_getglobal(L_, "compute_model_matrix");
        if (!lua_isfunction(L_, -1)) {
            lua_pop(L_, 1);
            return IdentityMatrix();
        }
    } else {
        lua_rawgeti(L_, LUA_REGISTRYINDEX, functionRef);
    }

    lua_pushnumber(L_, time);
    if (lua_pcall(L_, 1, 1, 0) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        throw std::runtime_error("Lua compute_model_matrix failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("'compute_model_matrix' did not return a table");
    }

    std::array<float, 16> matrix = ReadMatrix(L_, -1);
    lua_pop(L_, 1);
    return matrix;
}

std::array<float, 16> CubeScript::GetViewProjectionMatrix(float aspect) {
    lua_getglobal(L_, "get_view_projection");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("Lua function 'get_view_projection' is missing");
    }
    lua_pushnumber(L_, aspect);
    if (lua_pcall(L_, 1, 1, 0) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        throw std::runtime_error("Lua get_view_projection failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("'get_view_projection' did not return a table");
    }
    std::array<float, 16> matrix = ReadMatrix(L_, -1);
    lua_pop(L_, 1);
    return matrix;
}

std::vector<core::Vertex> CubeScript::ReadVertexArray(lua_State* L, int index) {
    int absIndex = lua_absindex(L, index);
    if (!lua_istable(L, absIndex)) {
        throw std::runtime_error("Expected table for vertex data");
    }

    size_t count = lua_rawlen(L, absIndex);
    std::vector<core::Vertex> vertices;
    vertices.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L, absIndex, static_cast<int>(i));
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Vertex entry at index " + std::to_string(i) + " is not a table");
        }

        int vertexIndex = lua_gettop(L);
        core::Vertex vertex{};

        lua_getfield(L, vertexIndex, "position");
        vertex.position = ReadVector3(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, vertexIndex, "color");
        vertex.color = ReadVector3(L, -1);
        lua_pop(L, 1);

        lua_pop(L, 1);
        vertices.push_back(vertex);
    }

    return vertices;
}

std::vector<uint16_t> CubeScript::ReadIndexArray(lua_State* L, int index) {
    int absIndex = lua_absindex(L, index);
    if (!lua_istable(L, absIndex)) {
        throw std::runtime_error("Expected table for index data");
    }

    size_t count = lua_rawlen(L, absIndex);
    std::vector<uint16_t> indices;
    indices.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L, absIndex, static_cast<int>(i));
        if (!lua_isinteger(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Index entry at position " + std::to_string(i) + " is not an integer");
        }
        lua_Integer value = lua_tointeger(L, -1);
        lua_pop(L, 1);
        if (value < 1) {
            throw std::runtime_error("Index values must be 1 or greater");
        }
        indices.push_back(static_cast<uint16_t>(value - 1));
    }

    return indices;
}

std::unordered_map<std::string, CubeScript::ShaderPaths> CubeScript::LoadShaderPathsMap() {
    lua_getglobal(L_, "get_shader_paths");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("Lua function 'get_shader_paths' is missing");
    }
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        throw std::runtime_error("Lua get_shader_paths failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("'get_shader_paths' did not return a table");
    }

    std::unordered_map<std::string, ShaderPaths> shaderMap;
    lua_pushnil(L_);
    while (lua_next(L_, -2) != 0) {
        if (lua_isstring(L_, -2) && lua_istable(L_, -1)) {
            std::string key = lua_tostring(L_, -2);
            shaderMap.emplace(key, ReadShaderPathsTable(L_, -1));
        }
        lua_pop(L_, 1);
    }

    lua_pop(L_, 1);
    if (shaderMap.empty()) {
        throw std::runtime_error("'get_shader_paths' did not return any shader variants");
    }
    return shaderMap;
}

CubeScript::ShaderPaths CubeScript::ReadShaderPathsTable(lua_State* L, int index) {
    ShaderPaths paths;
    int absIndex = lua_absindex(L, index);

    lua_getfield(L, absIndex, "vertex");
    if (!lua_isstring(L, -1)) {
        lua_pop(L, 1);
        throw std::runtime_error("Shader path 'vertex' must be a string");
    }
    paths.vertex = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, absIndex, "fragment");
    if (!lua_isstring(L, -1)) {
        lua_pop(L, 1);
        throw std::runtime_error("Shader path 'fragment' must be a string");
    }
    paths.fragment = lua_tostring(L, -1);
    lua_pop(L, 1);

    return paths;
}

std::array<float, 3> CubeScript::ReadVector3(lua_State* L, int index) {
    std::array<float, 3> result{};
    int absIndex = lua_absindex(L, index);
    size_t len = lua_rawlen(L, absIndex);
    if (len != 3) {
        throw std::runtime_error("Expected vector with 3 components");
    }
    for (size_t i = 1; i <= 3; ++i) {
        lua_rawgeti(L, absIndex, static_cast<int>(i));
        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Vector component is not a number");
        }
        result[i - 1] = static_cast<float>(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }
    return result;
}

std::array<float, 16> CubeScript::ReadMatrix(lua_State* L, int index) {
    std::array<float, 16> result{};
    int absIndex = lua_absindex(L, index);
    size_t len = lua_rawlen(L, absIndex);
    if (len != 16) {
        throw std::runtime_error("Expected 4x4 matrix with 16 components");
    }
    for (size_t i = 1; i <= 16; ++i) {
        lua_rawgeti(L, absIndex, static_cast<int>(i));
        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Matrix component is not a number");
        }
        result[i - 1] = static_cast<float>(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }
    return result;
}

std::string CubeScript::LuaErrorMessage(lua_State* L) {
    const char* message = lua_tostring(L, -1);
    return message ? message : "unknown lua error";
}

std::vector<CubeScript::GuiCommand> CubeScript::LoadGuiCommands() {
    std::vector<GuiCommand> commands;
    if (guiCommandsFnRef_ == LUA_REFNIL) {
        return commands;
    }
    lua_rawgeti(L_, LUA_REGISTRYINDEX, guiCommandsFnRef_);
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        throw std::runtime_error("Lua get_gui_commands failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("'get_gui_commands' did not return a table");
    }

    size_t count = lua_rawlen(L_, -1);
    commands.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L_, -1, static_cast<int>(i));
        if (!lua_istable(L_, -1)) {
            lua_pop(L_, 1);
            throw std::runtime_error("GUI command at index " + std::to_string(i) + " is not a table");
        }
        int commandIndex = lua_gettop(L_);
        lua_getfield(L_, commandIndex, "type");
        const char* typeName = lua_tostring(L_, -1);
        if (!typeName) {
            lua_pop(L_, 2);
            throw std::runtime_error("GUI command at index " + std::to_string(i) + " is missing a type");
        }
        GuiCommand command{};
        if (std::strcmp(typeName, "rect") == 0) {
            command.type = GuiCommand::Type::Rect;
            command.rect = ReadRect(L_, commandIndex);
            command.color = ReadColor(L_, commandIndex, GuiColor{0.0f, 0.0f, 0.0f, 1.0f});
            command.borderColor = ReadColor(L_, commandIndex, GuiColor{0.0f, 0.0f, 0.0f, 0.0f});
            lua_getfield(L_, commandIndex, "borderWidth");
            if (lua_isnumber(L_, -1)) {
                command.borderWidth = static_cast<float>(lua_tonumber(L_, -1));
            }
            lua_pop(L_, 1);
        } else if (std::strcmp(typeName, "text") == 0) {
            command.type = GuiCommand::Type::Text;
            ReadStringField(L_, commandIndex, "text", command.text);
            lua_getfield(L_, commandIndex, "fontSize");
            if (lua_isnumber(L_, -1)) {
                command.fontSize = static_cast<float>(lua_tonumber(L_, -1));
            }
            lua_pop(L_, 1);
            std::string align;
            if (ReadStringField(L_, commandIndex, "alignX", align)) {
                command.alignX = align;
            }
            if (ReadStringField(L_, commandIndex, "alignY", align)) {
                command.alignY = align;
            }
            lua_getfield(L_, commandIndex, "clipRect");
            if (lua_istable(L_, -1)) {
                command.clipRect = ReadRect(L_, -1);
                command.hasClipRect = true;
            }
            lua_pop(L_, 1);
            lua_getfield(L_, commandIndex, "bounds");
            if (lua_istable(L_, -1)) {
                command.bounds = ReadRect(L_, -1);
                command.hasBounds = true;
            }
            lua_pop(L_, 1);
            command.color = ReadColor(L_, commandIndex, GuiColor{1.0f, 1.0f, 1.0f, 1.0f});
        } else if (std::strcmp(typeName, "clip_push") == 0) {
            command.type = GuiCommand::Type::ClipPush;
            command.rect = ReadRect(L_, commandIndex);
        } else if (std::strcmp(typeName, "clip_pop") == 0) {
            command.type = GuiCommand::Type::ClipPop;
        } else if (std::strcmp(typeName, "svg") == 0) {
            command.type = GuiCommand::Type::Svg;
            ReadStringField(L_, commandIndex, "path", command.svgPath);
            command.rect = ReadRect(L_, commandIndex);
            command.svgTint = ReadColor(L_, commandIndex, GuiColor{1.0f, 1.0f, 1.0f, 0.0f});
            lua_getfield(L_, commandIndex, "tint");
            if (lua_istable(L_, -1)) {
                command.svgTint = ReadColor(L_, -1, command.svgTint);
            }
            lua_pop(L_, 1);
        }
        lua_pop(L_, 1); // pop type
        lua_pop(L_, 1); // pop command table
        commands.push_back(std::move(command));
    }

    lua_pop(L_, 1);
    return commands;
}

void CubeScript::UpdateGuiInput(const GuiInputSnapshot& input) {
    if (guiInputRef_ == LUA_REFNIL) {
        return;
    }
    lua_rawgeti(L_, LUA_REGISTRYINDEX, guiInputRef_);
    int stateIndex = lua_gettop(L_);

    lua_getfield(L_, stateIndex, "resetTransient");
    lua_pushvalue(L_, stateIndex);
    lua_call(L_, 1, 0);

    lua_getfield(L_, stateIndex, "setMouse");
    lua_pushvalue(L_, stateIndex);
    lua_pushnumber(L_, input.mouseX);
    lua_pushnumber(L_, input.mouseY);
    lua_pushboolean(L_, input.mouseDown);
    lua_call(L_, 4, 0);

    lua_getfield(L_, stateIndex, "setWheel");
    lua_pushvalue(L_, stateIndex);
    lua_pushnumber(L_, input.wheel);
    lua_call(L_, 2, 0);

    if (!input.textInput.empty()) {
        lua_getfield(L_, stateIndex, "addTextInput");
        lua_pushvalue(L_, stateIndex);
        lua_pushstring(L_, input.textInput.c_str());
        lua_call(L_, 2, 0);
    }

    for (const auto& [key, pressed] : input.keyStates) {
        lua_getfield(L_, stateIndex, "setKey");
        lua_pushvalue(L_, stateIndex);
        lua_pushstring(L_, key.c_str());
        lua_pushboolean(L_, pressed);
        lua_call(L_, 3, 0);
    }

    lua_pop(L_, 1);
}

bool CubeScript::HasGuiCommands() const {
    return guiCommandsFnRef_ != LUA_REFNIL;
}

std::filesystem::path CubeScript::GetScriptDirectory() const {
    return scriptDirectory_;
}

GuiRect CubeScript::ReadRect(lua_State* L, int index) {
    GuiRect rect{};
    if (!lua_istable(L, index)) {
        return rect;
    }
    int absIndex = lua_absindex(L, index);
    auto readField = [&](const char* name, float defaultValue) -> float {
        lua_getfield(L, absIndex, name);
        float value = defaultValue;
        if (lua_isnumber(L, -1)) {
            value = static_cast<float>(lua_tonumber(L, -1));
        }
        lua_pop(L, 1);
        return value;
    };
    rect.x = readField("x", rect.x);
    rect.y = readField("y", rect.y);
    rect.width = readField("width", rect.width);
    rect.height = readField("height", rect.height);
    return rect;
}

GuiColor CubeScript::ReadColor(lua_State* L, int index, const GuiColor& defaultColor) {
    GuiColor color = defaultColor;
    if (!lua_istable(L, index)) {
        return color;
    }
    int absIndex = lua_absindex(L, index);
    for (int component = 0; component < 4; ++component) {
        lua_rawgeti(L, absIndex, component + 1);
        if (lua_isnumber(L, -1)) {
            float value = static_cast<float>(lua_tonumber(L, -1));
            switch (component) {
                case 0: color.r = value; break;
                case 1: color.g = value; break;
                case 2: color.b = value; break;
                case 3: color.a = value; break;
            }
        }
        lua_pop(L, 1);
    }
    return color;
}

bool CubeScript::ReadStringField(lua_State* L, int index, const char* name, std::string& outString) {
    int absIndex = lua_absindex(L, index);
    lua_getfield(L, absIndex, name);
    if (lua_isstring(L, -1)) {
        outString = lua_tostring(L, -1);
        lua_pop(L, 1);
        return true;
    }
    lua_pop(L, 1);
    return false;
}

} // namespace sdl3cpp::script
