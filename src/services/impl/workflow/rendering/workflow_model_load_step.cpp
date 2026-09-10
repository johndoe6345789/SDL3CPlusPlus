#include "services/interfaces/workflow/rendering/workflow_model_load_step.hpp"
#include "services/interfaces/workflow/graphics/graphics_buffer_upload_helpers.hpp"
#include "services/interfaces/workflow/rendering/model_load_helpers.hpp"

#include <SDL3/SDL_gpu.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowModelLoadStep::WorkflowModelLoadStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowModelLoadStep::GetPluginId() const { return "model.load"; }

void WorkflowModelLoadStep::Execute(const WorkflowStepDefinition& step,
                                    WorkflowContext& context) {
    const ModelLoadParams params = ReadModelLoadParams(step, context);
    if (params.filePath.empty()) {
        throw std::runtime_error(
            "model.load: 'file_path' parameter is required");
    }

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error(
            "model.load: GPU device not found in context");
    }

    constexpr unsigned int kFlags =
        aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(params.filePath, kFlags);
    if (!scene || !scene->mRootNode ||
        (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        throw std::runtime_error(
            "model.load: Failed to load '" + params.filePath + "': " +
            importer.GetErrorString());
    }

    const AssimpMeshData mesh = ExtractAssimpMeshData(*scene, params.scale);
    if (mesh.vertices.empty()) {
        throw std::runtime_error(
            "model.load: No vertices found in '" + params.filePath + "'");
    }

    std::vector<uint8_t> vertexBytes(mesh.vertices.size() *
                                     sizeof(PosUvVertex));
    std::memcpy(vertexBytes.data(), mesh.vertices.data(), vertexBytes.size());
    // Throws on failure (see graphics.buffer.upload), matching
    // model.load's own error style.
    const UploadedGpuBuffers buffers =
        CreateAndUploadGpuBuffers(device, vertexBytes, mesh.indices);
    // Store using same convention as geometry.create_plane (plane_ prefix)
    context.Set<SDL_GPUBuffer*>("plane_" + params.name + "_vb",
                                buffers.vertexBuffer);
    context.Set<SDL_GPUBuffer*>("plane_" + params.name + "_ib",
                                buffers.indexBuffer);
    const auto vertexCount = static_cast<uint32_t>(mesh.vertices.size());
    const auto indexCount  = static_cast<uint32_t>(mesh.indices.size());
    const auto meshCount   = scene->mNumMeshes;
    context.Set("plane_" + params.name,
               BuildModelLoadMetadata(vertexCount, indexCount, meshCount,
                                      params.filePath));
    if (logger_) {
        logger_->Info(
            "model.load: '" + params.name + "' loaded from " +
            params.filePath + " (" + std::to_string(vertexCount) +
            " verts, " + std::to_string(indexCount) + " indices, " +
            std::to_string(meshCount) + " meshes)");
    }
}

}  // namespace sdl3cpp::services::impl
