#ifndef SDL3CPP_SCRIPT_LUA_HELPERS_HPP
#define SDL3CPP_SCRIPT_LUA_HELPERS_HPP

#include <array>
#include <string>

struct lua_State;

namespace sdl3cpp::script {

std::array<float, 3> ReadVector3(lua_State* L, int index);
std::array<float, 4> ReadQuaternion(lua_State* L, int index);
std::array<float, 16> ReadMatrix(lua_State* L, int index);
std::string GetLuaError(lua_State* L);
std::array<float, 16> IdentityMatrix();

} // namespace sdl3cpp::script

#endif // SDL3CPP_SCRIPT_LUA_HELPERS_HPP
