#pragma once

#include <array>
#include <string>

struct lua_State;

namespace sdl3cpp::services::impl::lua {

std::array<float, 3> ReadVector3(lua_State* L, int index);
std::array<float, 2> ReadVector2(lua_State* L, int index);
std::array<float, 4> ReadQuaternion(lua_State* L, int index);
std::array<float, 16> ReadMatrix(lua_State* L, int index);
std::string GetLuaError(lua_State* L);
std::array<float, 16> IdentityMatrix();
int LuaGlmMatrixIdentity(lua_State* L);
int LuaGlmMatrixMultiply(lua_State* L);
int LuaGlmMatrixTranslation(lua_State* L);
int LuaGlmMatrixRotationX(lua_State* L);
int LuaGlmMatrixRotationY(lua_State* L);
int LuaGlmMatrixLookAt(lua_State* L);
int LuaGlmMatrixPerspective(lua_State* L);
int LuaGlmMatrixFromTransform(lua_State* L);

}  // namespace sdl3cpp::services::impl::lua
