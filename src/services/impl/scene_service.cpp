#include "scene_service.hpp"
#include <limits>
#include <stdexcept>

namespace sdl3cpp::services::impl {

SceneService::SceneService(std::shared_ptr<ISceneScriptService> scriptService, std::shared_ptr<ILogger> logger)
    : scriptService_(scriptService), logger_(logger) {
    logger_->Trace("SceneService", "SceneService",
                   "scriptService=" + std::string(scriptService_ ? "set" : "null"));

    if (!scriptService_) {
        throw std::invalid_argument("Scene script service cannot be null");
    }
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

    combinedVertices_.clear();
    combinedIndices_.clear();
    drawInfos_.clear();

    if (objects.empty()) {
        initialized_ = false;
        return;
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

    combinedVertices_.reserve(totalVertices);
    combinedIndices_.reserve(totalIndices);
    drawInfos_.reserve(objects.size());

    for (const auto& obj : objects) {
        if (obj.vertices.empty() || obj.indices.empty()) {
            if (logger_) {
                logger_->Error("Scene object missing vertex or index data");
            }
            throw std::runtime_error("Scene object missing vertex or index data");
        }

        size_t vertexOffset = combinedVertices_.size();
        if (vertexOffset + obj.vertices.size() > kMaxIndexValue) {
            if (logger_) {
                logger_->Error("Scene vertex data exceeds uint16_t index range");
            }
            throw std::runtime_error("Scene vertex data exceeds uint16_t index range");
        }

        uint32_t indexOffset = static_cast<uint32_t>(combinedIndices_.size());
        combinedVertices_.insert(combinedVertices_.end(), obj.vertices.begin(), obj.vertices.end());
        combinedIndices_.reserve(combinedIndices_.size() + obj.indices.size());
        for (uint16_t index : obj.indices) {
            uint32_t adjusted = static_cast<uint32_t>(index) + static_cast<uint32_t>(vertexOffset);
            if (adjusted > kMaxIndexValue) {
                if (logger_) {
                    logger_->Error("Index offset exceeds uint16_t range");
                }
                throw std::runtime_error("Index offset exceeds uint16_t range");
            }
            combinedIndices_.push_back(index);
        }

        SceneDrawInfo drawInfo;
        drawInfo.indexOffset = indexOffset;
        drawInfo.indexCount = static_cast<uint32_t>(obj.indices.size());
        drawInfo.vertexOffset = static_cast<int32_t>(vertexOffset);
        drawInfo.computeModelMatrixRef = obj.computeModelMatrixRef;
        drawInfo.shaderKey = obj.shaderKey;
        drawInfos_.push_back(std::move(drawInfo));
    }

    if (logger_) {
        logger_->Trace("SceneService", "LoadScene",
                       "combinedVertices=" + std::to_string(combinedVertices_.size()) +
                       ", combinedIndices=" + std::to_string(combinedIndices_.size()) +
                       ", drawCalls=" + std::to_string(drawInfos_.size()));
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
        cmd.shaderKey = drawInfo.shaderKey;
        cmd.modelMatrix = scriptService_->ComputeModelMatrix(drawInfo.computeModelMatrixRef, time);
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

    combinedVertices_.clear();
    combinedIndices_.clear();
    drawInfos_.clear();
    initialized_ = false;
}

size_t SceneService::GetObjectCount() const {
    logger_->Trace("SceneService", "GetObjectCount");

    return drawInfos_.size();
}

void SceneService::Shutdown() noexcept {
    logger_->Trace("SceneService", "Shutdown");

    Clear();
}

}  // namespace sdl3cpp::services::impl
