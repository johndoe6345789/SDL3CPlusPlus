#pragma once

#include <array>
#include <string>

struct lua_State;

namespace sdl3cpp::services::impl::lua {

std::array<float, 3> ReadVector3(lua_State* L, int index);
std::array<float, 4> ReadQuaternion(lua_State* L, int index);
std::array<float, 16> ReadMatrix(lua_State* L, int index);
std::string GetLuaError(lua_State* L);
std::array<float, 16> IdentityMatrix();
int LuaGlmMatrixFromTransform(lua_State* L);

}  // namespace sdl3cpp::services::impl::lua
