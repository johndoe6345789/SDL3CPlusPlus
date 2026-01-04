#include "lua_helpers.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <lua.hpp>
#include <stdexcept>

namespace sdl3cpp::services::impl::lua {

std::array<float, 3> ReadVector3(lua_State* L, int index) {
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

std::array<float, 4> ReadQuaternion(lua_State* L, int index) {
    std::array<float, 4> result{};
    int absIndex = lua_absindex(L, index);
    size_t len = lua_rawlen(L, absIndex);
    if (len != 4) {
        throw std::runtime_error("Expected quaternion with 4 components");
    }
    for (size_t i = 1; i <= 4; ++i) {
        lua_rawgeti(L, absIndex, static_cast<int>(i));
        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Quaternion component is not a number");
        }
        result[i - 1] = static_cast<float>(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }
    return result;
}

std::array<float, 16> ReadMatrix(lua_State* L, int index) {
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

std::string GetLuaError(lua_State* L) {
    const char* message = lua_tostring(L, -1);
    return message ? message : "unknown lua error";
}

std::array<float, 16> IdentityMatrix() {
    return {1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f};
}

namespace {

glm::vec3 ToVec3(const std::array<float, 3>& value) {
    return glm::vec3(value[0], value[1], value[2]);
}

glm::quat ToQuat(const std::array<float, 4>& value) {
    return glm::quat(value[3], value[0], value[1], value[2]);
}

void PushMatrix(lua_State* L, const glm::mat4& matrix) {
    lua_newtable(L);
    const float* ptr = glm::value_ptr(matrix);
    for (int i = 0; i < 16; ++i) {
        lua_pushnumber(L, ptr[i]);
        lua_rawseti(L, -2, i + 1);
    }
}

}  // namespace

int LuaGlmMatrixFromTransform(lua_State* L) {
    std::array<float, 3> translation = ReadVector3(L, 1);
    std::array<float, 4> rotation = ReadQuaternion(L, 2);
    glm::vec3 pos = ToVec3(translation);
    glm::quat quat = ToQuat(rotation);
    glm::mat4 matrix = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(quat);
    PushMatrix(L, matrix);
    return 1;
}

}  // namespace sdl3cpp::services::impl::lua
