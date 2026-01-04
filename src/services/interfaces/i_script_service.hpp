#pragma once

#include "../../script/scene_manager.hpp"
#include "../../script/shader_manager.hpp"
#include "../../script/gui_types.hpp"
#include "../../script/physics_bridge.hpp"
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <array>
#include <string>

namespace sdl3cpp::services {

/**
 * @brief Script service interface.
 *
 * Provides Lua script execution and integration with scene, shaders, GUI, physics, and audio.
 */
class IScriptService {
public:
    virtual ~IScriptService() = default;

    // Scene management
    virtual std::vector<script::SceneManager::SceneObject> LoadSceneObjects() = 0;
    virtual std::array<float, 16> ComputeModelMatrix(int functionRef, float time) = 0;
    virtual std::array<float, 16> GetViewProjectionMatrix(float aspect) = 0;

    // Shader management
    virtual std::unordered_map<std::string, sdl3cpp::services::ShaderPaths> LoadShaderPathsMap() = 0;

    // GUI management
    virtual std::vector<script::GuiCommand> LoadGuiCommands() = 0;
    virtual void UpdateGuiInput(const script::GuiInputSnapshot& input) = 0;
    virtual bool HasGuiCommands() const = 0;

    // Physics bridge access
    virtual script::PhysicsBridge& GetPhysicsBridge() = 0;

    // Utility
    virtual std::filesystem::path GetScriptDirectory() const = 0;
    virtual std::string GetLuaError() = 0;
};

}  // namespace sdl3cpp::services
