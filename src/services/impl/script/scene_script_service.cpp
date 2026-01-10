#include "scene_script_service.hpp"

#include "lua_helpers.hpp"
#include "services/interfaces/i_logger.hpp"

#include <lua.hpp>

#include <array>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

std::array<float, 16> MultiplyMatrices(const std::array<float, 16>& left,
                                       const std::array<float, 16>& right) {
    glm::mat4 leftMat = glm::make_mat4(left.data());
    glm::mat4 rightMat = glm::make_mat4(right.data());
    glm::mat4 combined = leftMat * rightMat;
    std::array<float, 16> result{};
    std::memcpy(result.data(), glm::value_ptr(combined), sizeof(float) * result.size());
    return result;
}

bool ReadMatrixField(lua_State* L, int tableIndex, const char* field, std::array<float, 16>& target) {
    lua_getfield(L, tableIndex, field);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return false;
    }
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        throw std::runtime_error(std::string("Field '") + field + "' must be a 4x4 matrix");
    }
    target = lua::ReadMatrix(L, -1);
    lua_pop(L, 1);
    return true;
}

bool ReadVector3Field(lua_State* L, int tableIndex, const char* field, std::array<float, 3>& target) {
    lua_getfield(L, tableIndex, field);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return false;
    }
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        throw std::runtime_error(std::string("Field '") + field + "' must be a vec3");
    }
    target = lua::ReadVector3(L, -1);
    lua_pop(L, 1);
    return true;
}

std::vector<core::Vertex> ReadVertexArray(lua_State* L, int index, const std::shared_ptr<ILogger>& logger) {
    int absIndex = lua_absindex(L, index);
    if (!lua_istable(L, absIndex)) {
        if (logger) {
            logger->Error("Expected table for vertex data");
        }
        throw std::runtime_error("Expected table for vertex data");
    }

    size_t count = lua_rawlen(L, absIndex);
    std::vector<core::Vertex> vertices;
    vertices.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L, absIndex, static_cast<int>(i));
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            if (logger) {
                logger->Error("Vertex entry at index " + std::to_string(i) + " is not a table");
            }
            throw std::runtime_error("Vertex entry at index " + std::to_string(i) + " is not a table");
        }

        int vertexIndex = lua_gettop(L);
        core::Vertex vertex{};

        lua_getfield(L, vertexIndex, "position");
        vertex.position = lua::ReadVector3(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, vertexIndex, "normal");
        if (lua_istable(L, -1)) {
            vertex.normal = lua::ReadVector3(L, -1);
        } else {
            vertex.normal = {0.0f, 0.0f, 1.0f};
        }
        lua_pop(L, 1);

        lua_getfield(L, vertexIndex, "tangent");
        if (lua_istable(L, -1)) {
            vertex.tangent = lua::ReadVector3(L, -1);
        } else {
            vertex.tangent = {1.0f, 0.0f, 0.0f};
        }
        lua_pop(L, 1);

        lua_getfield(L, vertexIndex, "color");
        vertex.color = lua::ReadVector3(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, vertexIndex, "texcoord");
        if (lua_istable(L, -1)) {
            vertex.texcoord = lua::ReadVector2(L, -1);
        } else {
            vertex.texcoord = {0.0f, 0.0f};
        }
        lua_pop(L, 1);

        lua_pop(L, 1);
        vertices.push_back(vertex);
    }

    return vertices;
}

std::vector<uint16_t> ReadIndexArray(lua_State* L, int index, const std::shared_ptr<ILogger>& logger) {
    int absIndex = lua_absindex(L, index);
    if (!lua_istable(L, absIndex)) {
        if (logger) {
            logger->Error("Expected table for index data");
        }
        throw std::runtime_error("Expected table for index data");
    }

    size_t count = lua_rawlen(L, absIndex);
    std::vector<uint16_t> indices;
    indices.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L, absIndex, static_cast<int>(i));
        if (!lua_isinteger(L, -1)) {
            lua_pop(L, 1);
            if (logger) {
                logger->Error("Index entry at position " + std::to_string(i) + " is not an integer");
            }
            throw std::runtime_error("Index entry at position " + std::to_string(i) + " is not an integer");
        }
        lua_Integer value = lua_tointeger(L, -1);
        lua_pop(L, 1);
        if (value < 1) {
            if (logger) {
                logger->Error("Index values must be 1 or greater");
            }
            throw std::runtime_error("Index values must be 1 or greater");
        }
        indices.push_back(static_cast<uint16_t>(value - 1));
    }

    return indices;
}

}  // namespace

SceneScriptService::SceneScriptService(std::shared_ptr<IScriptEngineService> engineService,
                                       std::shared_ptr<ILogger> logger)
    : engineService_(std::move(engineService)),
      logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("SceneScriptService", "SceneScriptService",
                       "engineService=" + std::string(engineService_ ? "set" : "null"));
    }
}

std::vector<SceneObject> SceneScriptService::LoadSceneObjects() {
    if (logger_) {
        logger_->Trace("SceneScriptService", "LoadSceneObjects");
    }
    lua_State* L = GetLuaState();

    lua_getglobal(L, "get_scene_objects");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("Lua function 'get_scene_objects' is missing");
        }
        throw std::runtime_error("Lua function 'get_scene_objects' is missing");
    }
    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        std::string message = lua::GetLuaError(L);
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("Lua get_scene_objects failed: " + message);
        }
        throw std::runtime_error("Lua get_scene_objects failed: " + message);
    }
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("'get_scene_objects' did not return a table");
        }
        throw std::runtime_error("'get_scene_objects' did not return a table");
    }

    size_t count = lua_rawlen(L, -1);
    std::vector<SceneObject> objects;
    objects.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L, -1, static_cast<int>(i));
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            if (logger_) {
                logger_->Error("Scene object at index " + std::to_string(i) + " is not a table");
            }
            throw std::runtime_error("Scene object at index " + std::to_string(i) + " is not a table");
        }

        SceneObject object;
        lua_getfield(L, -1, "vertices");
        object.vertices = ReadVertexArray(L, -1, logger_);
        lua_pop(L, 1);
        if (object.vertices.empty()) {
            lua_pop(L, 1);
            if (logger_) {
                logger_->Error("Scene object " + std::to_string(i) + " must supply at least one vertex");
            }
            throw std::runtime_error("Scene object " + std::to_string(i) + " must supply at least one vertex");
        }

        lua_getfield(L, -1, "indices");
        object.indices = ReadIndexArray(L, -1, logger_);
        lua_pop(L, 1);
        if (object.indices.empty()) {
            lua_pop(L, 1);
            if (logger_) {
                logger_->Error("Scene object " + std::to_string(i) + " must supply indices");
            }
            throw std::runtime_error("Scene object " + std::to_string(i) + " must supply indices");
        }

        lua_getfield(L, -1, "compute_model_matrix");
        if (lua_isfunction(L, -1)) {
            object.computeModelMatrixRef = luaL_ref(L, LUA_REGISTRYINDEX);
        } else {
            lua_pop(L, 1);
            object.computeModelMatrixRef = -1;
        }

        object.shaderKeys.clear();
        lua_getfield(L, -1, "shader_keys");
        if (lua_istable(L, -1)) {
            const size_t shaderKeyCount = lua_rawlen(L, -1);
            object.shaderKeys.reserve(shaderKeyCount);
            for (size_t keyIndex = 1; keyIndex <= shaderKeyCount; ++keyIndex) {
                lua_rawgeti(L, -1, static_cast<int>(keyIndex));
                if (lua_isstring(L, -1)) {
                    object.shaderKeys.emplace_back(lua_tostring(L, -1));
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);

        if (object.shaderKeys.empty()) {
            lua_getfield(L, -1, "shader_key");
            if (lua_isstring(L, -1)) {
                object.shaderKeys.emplace_back(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }

        lua_getfield(L, -1, "object_type");
        if (lua_isstring(L, -1)) {
            object.objectType = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        objects.push_back(std::move(object));
        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    return objects;
}

std::array<float, 16> SceneScriptService::ComputeModelMatrix(int functionRef, float time) {
    if (logger_) {
        logger_->Trace("SceneScriptService", "ComputeModelMatrix",
                       "functionRef=" + std::to_string(functionRef) +
                       ", time=" + std::to_string(time));
    }
    lua_State* L = GetLuaState();

    if (functionRef < 0) {
        lua_getglobal(L, "compute_model_matrix");
        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 1);
            return lua::IdentityMatrix();
        }
    } else {
        lua_rawgeti(L, LUA_REGISTRYINDEX, functionRef);
    }

    lua_pushnumber(L, time);
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        std::string message = lua::GetLuaError(L);
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("Lua compute_model_matrix failed: " + message);
        }
        throw std::runtime_error("Lua compute_model_matrix failed: " + message);
    }
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("'compute_model_matrix' did not return a table");
        }
        throw std::runtime_error("'compute_model_matrix' did not return a table");
    }

    std::array<float, 16> matrix = lua::ReadMatrix(L, -1);
    lua_pop(L, 1);
    return matrix;
}

ViewState SceneScriptService::GetViewState(float aspect) {
    if (logger_) {
        logger_->Trace("SceneScriptService", "GetViewState", "aspect=" + std::to_string(aspect));
    }
    lua_State* L = GetLuaState();

    ViewState state;
    state.view = lua::IdentityMatrix();
    state.proj = lua::IdentityMatrix();
    state.viewProj = lua::IdentityMatrix();
    state.cameraPosition = {0.0f, 0.0f, 0.0f};

    lua_getglobal(L, "get_view_state");
    if (lua_isfunction(L, -1)) {
        lua_pushnumber(L, aspect);
        if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
            std::string message = lua::GetLuaError(L);
            lua_pop(L, 1);
            if (logger_) {
                logger_->Error("Lua get_view_state failed: " + message);
            }
            throw std::runtime_error("Lua get_view_state failed: " + message);
        }
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            if (logger_) {
                logger_->Error("'get_view_state' did not return a table");
            }
            throw std::runtime_error("'get_view_state' did not return a table");
        }
        bool hasView = false;
        bool hasProj = false;
        bool hasViewProj = false;

        try {
            hasView = ReadMatrixField(L, -1, "view", state.view);
            hasProj = ReadMatrixField(L, -1, "proj", state.proj);
            hasViewProj = ReadMatrixField(L, -1, "view_proj", state.viewProj);
            if (!hasViewProj) {
                hasViewProj = ReadMatrixField(L, -1, "viewProj", state.viewProj);
            }
            ReadVector3Field(L, -1, "camera_pos", state.cameraPosition);
            ReadVector3Field(L, -1, "camera_position", state.cameraPosition);
        } catch (const std::exception& ex) {
            lua_pop(L, 1);
            if (logger_) {
                logger_->Error("Lua get_view_state returned invalid data: " + std::string(ex.what()));
            }
            throw;
        }

        lua_pop(L, 1);

        if (!hasViewProj && hasView && hasProj) {
            state.viewProj = MultiplyMatrices(state.proj, state.view);
        }
        return state;
    }

    lua_pop(L, 1);
    lua_getglobal(L, "get_view_projection");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("Lua function 'get_view_projection' is missing");
        }
        throw std::runtime_error("Lua function 'get_view_projection' is missing");
    }
    lua_pushnumber(L, aspect);
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        std::string message = lua::GetLuaError(L);
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("Lua get_view_projection failed: " + message);
        }
        throw std::runtime_error("Lua get_view_projection failed: " + message);
    }
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("'get_view_projection' did not return a table");
        }
        throw std::runtime_error("'get_view_projection' did not return a table");
    }
    state.viewProj = lua::ReadMatrix(L, -1);
    lua_pop(L, 1);
    return state;
}

lua_State* SceneScriptService::GetLuaState() const {
    if (logger_) {
        logger_->Trace("SceneScriptService", "GetLuaState");
    }
    if (!engineService_) {
        throw std::runtime_error("Scene script service is missing script engine service");
    }
    lua_State* state = engineService_->GetLuaState();
    if (!state) {
        throw std::runtime_error("Lua state is not initialized");
    }
    return state;
}

}  // namespace sdl3cpp::services::impl
