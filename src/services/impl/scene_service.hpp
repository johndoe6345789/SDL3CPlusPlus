#pragma once

#include "../interfaces/i_scene_service.hpp"
#include "../interfaces/i_script_service.hpp"
#include "../../di/lifecycle.hpp"
#include <vector>
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Scene service implementation.
 *
 * Maintains scene graph state and generates render commands.
 * Separated from script service to decouple scene state from Lua execution.
 */
class SceneService : public ISceneService,
                     public di::IShutdownable {
public:
    explicit SceneService(std::shared_ptr<IScriptService> scriptService);
    ~SceneService() override;

    // ISceneService interface
    void LoadScene(const std::vector<SceneObject>& objects) override;
    void UpdateScene(float deltaTime) override;
    std::vector<RenderCommand> GetRenderCommands(float time) const override;
    void Clear() override;
    size_t GetObjectCount() const override;

    // IShutdownable interface
    void Shutdown() noexcept override;

private:
    std::shared_ptr<IScriptService> scriptService_;
    std::vector<SceneObject> sceneObjects_;
    bool initialized_ = false;
};

}  // namespace sdl3cpp::services::impl