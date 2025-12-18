#include "script/cube_script.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::script {

CubeScript::CubeScript(const std::filesystem::path& scriptPath) : L_(luaL_newstate()) {
    if (!L_) {
        throw std::runtime_error("Failed to create Lua state");
    }
    luaL_openlibs(L_);
    if (luaL_dofile(L_, scriptPath.string().c_str()) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        lua_close(L_);
        L_ = nullptr;
        throw std::runtime_error("Failed to load Lua script: " + message);
    }
}

CubeScript::~CubeScript() {
    if (L_) {
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
            return core::IdentityMatrix();
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

} // namespace sdl3cpp::script
