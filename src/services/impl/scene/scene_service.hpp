#pragma once

#include "services/interfaces/i_scene_service.hpp"
#include "services/interfaces/i_ecs_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_probe_service.hpp"
#include "../../../di/lifecycle.hpp"
#include <array>
#include <entt/entt.hpp>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Scene service implementation.
 *
 * Maintains scene graph state and generates render commands.
 * Separated from script execution to keep scene state deterministic.
 */
class SceneService : public ISceneService,
                     public di::IShutdownable {
public:
    SceneService(std::shared_ptr<IEcsService> ecsService,
                 std::shared_ptr<ILogger> logger,
                 std::shared_ptr<IProbeService> probeService = nullptr);
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
    struct SceneTag {};

    struct MeshComponent {
        std::vector<core::Vertex> vertices;
        std::vector<uint16_t> indices;
    };

    struct RenderComponent {
        int computeModelMatrixRef = -1;
        bool hasCustomModelMatrix = false;
        std::array<float, 16> modelMatrix{};
        std::vector<std::string> shaderKeys;
    };

    struct SceneDrawInfo {
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
        int32_t vertexOffset = 0;
        int computeModelMatrixRef = -1;
        bool hasCustomModelMatrix = false;
        std::array<float, 16> modelMatrix{};
        std::vector<std::string> shaderKeys;
    };

    bool ShouldEmitRuntimeProbe() const;
    void ReportRuntimeProbe(const std::string& code,
                            const std::string& message,
                            const std::string& details) const;
    void ClearSceneEntities();

    std::shared_ptr<IEcsService> ecsService_;
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IProbeService> probeService_;
    entt::registry* registry_ = nullptr;
    std::vector<entt::entity> sceneEntities_;
    std::vector<core::Vertex> combinedVertices_;
    std::vector<uint16_t> combinedIndices_;
    std::vector<SceneDrawInfo> drawInfos_;
    bool initialized_ = false;
    size_t lastSceneObjectCount_ = 0;
    size_t lastSceneVertexCount_ = 0;
    size_t lastSceneIndexCount_ = 0;
    bool hasSceneSignature_ = false;
};

}  // namespace sdl3cpp::services::impl
