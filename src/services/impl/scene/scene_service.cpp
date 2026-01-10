#include "scene_service.hpp"
#include <limits>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

SceneService::SceneService(std::shared_ptr<IEcsService> ecsService,
                           std::shared_ptr<ILogger> logger,
                           std::shared_ptr<IProbeService> probeService)
    : ecsService_(std::move(ecsService)),
      logger_(std::move(logger)),
      probeService_(std::move(probeService)) {
    logger_->Trace("SceneService", "SceneService",
                   "ecsService=" + std::string(ecsService_ ? "set" : "null") +
                   ", probeService=" + std::string(probeService_ ? "set" : "null"));

    if (!ecsService_) {
        throw std::invalid_argument("ECS service cannot be null");
    }
    registry_ = &ecsService_->GetRegistry();
}

SceneService::~SceneService() {
    logger_->Trace("SceneService", "~SceneService");
    if (initialized_) {
        Shutdown();
    }
}

void SceneService::LoadScene(const std::vector<SceneObject>& objects) {
    logger_->Trace("SceneService", "LoadScene",
                   "objects.size=" + std::to_string(objects.size()));

    ClearSceneEntities();
    combinedVertices_.clear();
    combinedIndices_.clear();
    drawInfos_.clear();

    if (objects.empty()) {
        initialized_ = false;
        hasSceneSignature_ = false;
        return;
    }

    if (!registry_) {
        throw std::runtime_error("ECS registry is not available");
    }

    size_t totalVertices = 0;
    size_t totalIndices = 0;
    for (const auto& obj : objects) {
        totalVertices += obj.vertices.size();
        totalIndices += obj.indices.size();
    }

    constexpr size_t kMaxIndexValue = std::numeric_limits<uint16_t>::max();
    if (totalVertices > kMaxIndexValue) {
        if (logger_) {
            logger_->Error("Scene vertex count exceeds uint16_t index range");
        }
        throw std::runtime_error("Scene vertex count exceeds uint16_t index range");
    }

    const bool shouldReportSceneLoad = ShouldEmitRuntimeProbe() &&
        (!hasSceneSignature_ ||
         objects.size() != lastSceneObjectCount_ ||
         totalVertices != lastSceneVertexCount_ ||
         totalIndices != lastSceneIndexCount_);

    combinedVertices_.reserve(totalVertices);
    combinedIndices_.reserve(totalIndices);
    drawInfos_.reserve(objects.size());
    sceneEntities_.reserve(objects.size());

    for (const auto& obj : objects) {
        if (obj.vertices.empty() || obj.indices.empty()) {
            if (logger_) {
                logger_->Error("Scene object missing vertex or index data");
            }
            throw std::runtime_error("Scene object missing vertex or index data");
        }
        if (obj.shaderKeys.empty()) {
            if (logger_) {
                logger_->Error("Scene object missing shader keys");
            }
            throw std::runtime_error("Scene object missing shader keys");
        }

        auto entity = registry_->create();
        sceneEntities_.push_back(entity);
        registry_->emplace<SceneTag>(entity);
        registry_->emplace<MeshComponent>(entity, obj.vertices, obj.indices);
        RenderComponent renderComponent;
        renderComponent.computeModelMatrixRef = obj.computeModelMatrixRef;
        renderComponent.hasCustomModelMatrix = obj.hasCustomModelMatrix;
        renderComponent.modelMatrix = obj.modelMatrix;
        renderComponent.shaderKeys = obj.shaderKeys;
        registry_->emplace<RenderComponent>(entity, std::move(renderComponent));
    }

    for (const auto entity : sceneEntities_) {
        const auto& mesh = registry_->get<MeshComponent>(entity);
        const auto& render = registry_->get<RenderComponent>(entity);

        size_t vertexOffset = combinedVertices_.size();
        if (vertexOffset + mesh.vertices.size() > kMaxIndexValue) {
            if (logger_) {
                logger_->Error("Scene vertex data exceeds uint16_t index range");
            }
            throw std::runtime_error("Scene vertex data exceeds uint16_t index range");
        }

        uint32_t indexOffset = static_cast<uint32_t>(combinedIndices_.size());
        combinedVertices_.insert(combinedVertices_.end(), mesh.vertices.begin(), mesh.vertices.end());
        combinedIndices_.reserve(combinedIndices_.size() + mesh.indices.size());
        if (logger_) {
            logger_->Trace("SceneService", "LoadScene",
                           "Remapping indices for entity=" + std::to_string(entt::to_integral(entity)) +
                           ", vertexOffset=" + std::to_string(vertexOffset) +
                           ", indexCount=" + std::to_string(mesh.indices.size()));
        }
        for (uint16_t index : mesh.indices) {
            uint32_t adjusted = static_cast<uint32_t>(index) + static_cast<uint32_t>(vertexOffset);
            if (adjusted > kMaxIndexValue) {
                if (logger_) {
                    logger_->Error("Index offset exceeds uint16_t range");
                }
                throw std::runtime_error("Index offset exceeds uint16_t range");
            }
            combinedIndices_.push_back(static_cast<uint16_t>(adjusted));
        }

        SceneDrawInfo drawInfo;
        drawInfo.indexOffset = indexOffset;
        drawInfo.indexCount = static_cast<uint32_t>(mesh.indices.size());
        drawInfo.vertexOffset = static_cast<int32_t>(vertexOffset);
        drawInfo.computeModelMatrixRef = render.computeModelMatrixRef;
        drawInfo.hasCustomModelMatrix = render.hasCustomModelMatrix;
        drawInfo.modelMatrix = render.modelMatrix;
        drawInfo.shaderKeys = render.shaderKeys;
        drawInfos_.push_back(std::move(drawInfo));
    }

    if (logger_) {
        logger_->Trace("SceneService", "LoadScene",
                       "combinedVertices=" + std::to_string(combinedVertices_.size()) +
                       ", combinedIndices=" + std::to_string(combinedIndices_.size()) +
                       ", drawCalls=" + std::to_string(drawInfos_.size()));
    }
    lastSceneObjectCount_ = objects.size();
    lastSceneVertexCount_ = totalVertices;
    lastSceneIndexCount_ = totalIndices;
    hasSceneSignature_ = true;
    if (shouldReportSceneLoad) {
        ReportRuntimeProbe("SCENE_LOAD",
                           "Scene loaded",
                           "objects=" + std::to_string(objects.size()) +
                               ", vertices=" + std::to_string(totalVertices) +
                               ", indices=" + std::to_string(totalIndices));
    }
    initialized_ = true;
}

void SceneService::UpdateScene(float deltaTime) {
    logger_->Trace("SceneService", "UpdateScene",
                   "deltaTime=" + std::to_string(deltaTime));

    // Scene updates would go here (animations, physics, etc.)
    // For now, this is a placeholder
    (void)deltaTime;
}

std::vector<RenderCommand> SceneService::GetRenderCommands(float time) const {
    logger_->Trace("SceneService", "GetRenderCommands",
                   "time=" + std::to_string(time));

    if (!initialized_) {
        return {};
    }

    std::vector<RenderCommand> commands;
    commands.reserve(drawInfos_.size());

    for (const auto& drawInfo : drawInfos_) {
        RenderCommand cmd;
        cmd.indexOffset = drawInfo.indexOffset;
        cmd.indexCount = drawInfo.indexCount;
        cmd.vertexOffset = drawInfo.vertexOffset;
        cmd.shaderKeys = drawInfo.shaderKeys;
        cmd.modelMatrix = drawInfo.hasCustomModelMatrix
            ? drawInfo.modelMatrix
            : std::array<float, 16>{
                  1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f
              };
        commands.push_back(cmd);
    }

    return commands;
}

const std::vector<core::Vertex>& SceneService::GetCombinedVertices() const {
    logger_->Trace("SceneService", "GetCombinedVertices");
    return combinedVertices_;
}

const std::vector<uint16_t>& SceneService::GetCombinedIndices() const {
    logger_->Trace("SceneService", "GetCombinedIndices");
    return combinedIndices_;
}

void SceneService::Clear() {
    logger_->Trace("SceneService", "Clear");

    ClearSceneEntities();
    combinedVertices_.clear();
    combinedIndices_.clear();
    drawInfos_.clear();
    initialized_ = false;
}

size_t SceneService::GetObjectCount() const {
    logger_->Trace("SceneService", "GetObjectCount");

    return sceneEntities_.size();
}

void SceneService::Shutdown() noexcept {
    logger_->Trace("SceneService", "Shutdown");

    Clear();
}

void SceneService::ClearSceneEntities() {
    if (!registry_ || sceneEntities_.empty()) {
        sceneEntities_.clear();
        return;
    }

    for (const auto entity : sceneEntities_) {
        if (registry_->valid(entity)) {
            registry_->destroy(entity);
        }
    }
    sceneEntities_.clear();
}

bool SceneService::ShouldEmitRuntimeProbe() const {
    if (!probeService_ || !logger_) {
        return false;
    }
    LogLevel level = logger_->GetLevel();
    return level == LogLevel::TRACE || level == LogLevel::DEBUG;
}

void SceneService::ReportRuntimeProbe(const std::string& code,
                                      const std::string& message,
                                      const std::string& details) const {
    ProbeReport report{};
    report.severity = ProbeSeverity::Info;
    report.code = code;
    report.message = message;
    report.details = details;
    probeService_->Report(report);
}

}  // namespace sdl3cpp::services::impl
