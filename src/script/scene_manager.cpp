#include "script/scene_manager.hpp"
#include "logging/logger.hpp"

#include <lua.hpp>

#include <array>
#include <stdexcept>
#include <string>

namespace sdl3cpp::script {

SceneManager::SceneManager(lua_State* L) : L_(L) {
    sdl3cpp::logging::TraceGuard trace;
}

std::vector<SceneManager::SceneObject> SceneManager::LoadSceneObjects() {
    lua_getglobal(L_, "get_scene_objects");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("Lua function 'get_scene_objects' is missing");
    }
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        std::string message = GetLuaError();
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
        object.vertices = ReadVertexArray(-1);
        lua_pop(L_, 1);
        if (object.vertices.empty()) {
            lua_pop(L_, 1);
            throw std::runtime_error("Scene object " + std::to_string(i) + " must supply at least one vertex");
        }

        lua_getfield(L_, -1, "indices");
        object.indices = ReadIndexArray(-1);
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

std::array<float, 16> SceneManager::ComputeModelMatrix(int functionRef, float time) {
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
        std::string message = GetLuaError();
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

std::array<float, 16> SceneManager::GetViewProjectionMatrix(float aspect) {
    lua_getglobal(L_, "get_view_projection");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("Lua function 'get_view_projection' is missing");
    }
    lua_pushnumber(L_, aspect);
    if (lua_pcall(L_, 1, 1, 0) != LUA_OK) {
        std::string message = GetLuaError();
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

std::vector<core::Vertex> SceneManager::ReadVertexArray(int index) {
    int absIndex = lua_absindex(L_, index);
    if (!lua_istable(L_, absIndex)) {
        throw std::runtime_error("Expected table for vertex data");
    }

    size_t count = lua_rawlen(L_, absIndex);
    std::vector<core::Vertex> vertices;
    vertices.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L_, absIndex, static_cast<int>(i));
        if (!lua_istable(L_, -1)) {
            lua_pop(L_, 1);
            throw std::runtime_error("Vertex entry at index " + std::to_string(i) + " is not a table");
        }

        int vertexIndex = lua_gettop(L_);
        core::Vertex vertex{};

        lua_getfield(L_, vertexIndex, "position");
        vertex.position = ReadVector3(L_, -1);
        lua_pop(L_, 1);

        lua_getfield(L_, vertexIndex, "color");
        vertex.color = ReadVector3(L_, -1);
        lua_pop(L_, 1);

        lua_pop(L_, 1);
        vertices.push_back(vertex);
    }

    return vertices;
}

std::vector<uint16_t> SceneManager::ReadIndexArray(int index) {
    int absIndex = lua_absindex(L_, index);
    if (!lua_istable(L_, absIndex)) {
        throw std::runtime_error("Expected table for index data");
    }

    size_t count = lua_rawlen(L_, absIndex);
    std::vector<uint16_t> indices;
    indices.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L_, absIndex, static_cast<int>(i));
        if (!lua_isinteger(L_, -1)) {
            lua_pop(L_, 1);
            throw std::runtime_error("Index entry at position " + std::to_string(i) + " is not an integer");
        }
        lua_Integer value = lua_tointeger(L_, -1);
        lua_pop(L_, 1);
        if (value < 1) {
            throw std::runtime_error("Index values must be 1 or greater");
        }
        indices.push_back(static_cast<uint16_t>(value - 1));
    }

    return indices;
}

std::string SceneManager::GetLuaError() {
    const char* message = lua_tostring(L_, -1);
    return message ? message : "unknown lua error";
}

} // namespace sdl3cpp::script