#pragma once

#include "../interfaces/i_scene_service.hpp"
#include "../interfaces/i_scene_script_service.hpp"
#include "../interfaces/i_logger.hpp"
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
    explicit SceneService(std::shared_ptr<ISceneScriptService> scriptService, std::shared_ptr<ILogger> logger);
    ~SceneService() override;

    // ISceneService interface
    void LoadScene(const std::vector<SceneObject>& objects) override;
    void UpdateScene(float deltaTime) override;
    std::vector<RenderCommand> GetRenderCommands(float time) const override;
    const std::vector<core::Vertex>& GetCombinedVertices() const override;
    const std::vector<uint16_t>& GetCombinedIndices() const override;
    void Clear() override;
    size_t GetObjectCount() const override;

    // IShutdownable interface
    void Shutdown() noexcept override;

private:
    struct SceneDrawInfo {
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
        int32_t vertexOffset = 0;
        int computeModelMatrixRef = -1;
        std::string shaderKey;
    };

    std::shared_ptr<ISceneScriptService> scriptService_;
    std::shared_ptr<ILogger> logger_;
    std::vector<core::Vertex> combinedVertices_;
    std::vector<uint16_t> combinedIndices_;
    std::vector<SceneDrawInfo> drawInfos_;
    bool initialized_ = false;
};

}  // namespace sdl3cpp::services::impl
