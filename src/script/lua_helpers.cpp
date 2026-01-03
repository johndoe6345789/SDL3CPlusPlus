#include "script/lua_helpers.hpp"

#include <lua.hpp>
#include <stdexcept>

namespace sdl3cpp::script {

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

} // namespace sdl3cpp::script
