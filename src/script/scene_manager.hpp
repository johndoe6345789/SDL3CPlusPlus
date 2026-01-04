#pragma once

#include <lua.hpp>
#include <lauxlib.h>
#include "script/lua_helpers.hpp"
#include "core/vertex.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services {
class ILogger;
}

namespace sdl3cpp::script {

class SceneManager {
public:
    struct SceneObject {
        std::vector<core::Vertex> vertices;
        std::vector<uint16_t> indices;
        int computeModelMatrixRef = LUA_REFNIL;
        std::string shaderKey = "default";
    };

    SceneManager(lua_State* L, std::shared_ptr<services::ILogger> logger);

    std::vector<SceneObject> LoadSceneObjects();
    std::array<float, 16> ComputeModelMatrix(int functionRef, float time);
    std::array<float, 16> GetViewProjectionMatrix(float aspect);

private:
    lua_State* L_;

    std::vector<core::Vertex> ReadVertexArray(int index);
    std::vector<uint16_t> ReadIndexArray(int index);
    std::string GetLuaError();

    std::shared_ptr<services::ILogger> logger_;
};

} // namespace sdl3cpp::script
