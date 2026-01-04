#pragma once

#include "i_graphics_service.hpp"
#include "scene_types.hpp"
#include <vector>

namespace sdl3cpp::services {

/**
 * @brief Scene management service interface.
 *
 * Maintains the scene graph and generates render commands for the graphics service.
 * Separated from script service to decouple scene state from Lua execution.
 */
class ISceneService {
public:
    virtual ~ISceneService() = default;

    /**
     * @brief Load a scene from scene objects.
     *
     * Replaces the current scene with new objects.
     *
     * @param objects Vector of scene objects to load
     */
    virtual void LoadScene(const std::vector<SceneObject>& objects) = 0;

    /**
     * @brief Update scene state (animations, transformations, etc.).
     *
     * @param deltaTime Time since last update in seconds
     */
    virtual void UpdateScene(float deltaTime) = 0;

    /**
     * @brief Generate render commands for the current scene.
     *
     * Evaluates model matrices and prepares draw calls.
     *
     * @param time Current time in seconds
     * @return Vector of render commands for the graphics service
     */
    virtual std::vector<RenderCommand> GetRenderCommands(float time) const = 0;

    /**
     * @brief Clear the scene (remove all objects).
     */
    virtual void Clear() = 0;

    /**
     * @brief Get the number of objects in the scene.
     *
     * @return Object count
     */
    virtual size_t GetObjectCount() const = 0;
};

}  // namespace sdl3cpp::services
