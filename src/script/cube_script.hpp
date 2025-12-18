#ifndef SDL3CPP_SCRIPT_CUBE_SCRIPT_HPP
#define SDL3CPP_SCRIPT_CUBE_SCRIPT_HPP

#include <array>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <lua.hpp>

#include "core/math.hpp"

namespace sdl3cpp::script {

class CubeScript {
public:
    explicit CubeScript(const std::filesystem::path& scriptPath);
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
    std::unordered_map<std::string, ShaderPaths> LoadShaderPathsMap();

private:
    static std::array<float, 3> ReadVector3(lua_State* L, int index);
    static std::array<float, 16> ReadMatrix(lua_State* L, int index);
    static std::vector<core::Vertex> ReadVertexArray(lua_State* L, int index);
    static std::vector<uint16_t> ReadIndexArray(lua_State* L, int index);
    static std::string LuaErrorMessage(lua_State* L);
    static ShaderPaths ReadShaderPathsTable(lua_State* L, int index);

    lua_State* L_ = nullptr;
};

} // namespace sdl3cpp::script

#endif // SDL3CPP_SCRIPT_CUBE_SCRIPT_HPP
